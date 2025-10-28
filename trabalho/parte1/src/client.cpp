#include "client.hpp"
#include "udp_util.hpp"
#include "utils.hpp"
#include "server_state.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <csignal>
#include <atomic>
#include <chrono>
#include <fstream>
#include <queue>

using namespace std;

Client* g_client_instance = nullptr;

void signal_handler(int signum) {
    if (g_client_instance) {
        g_client_instance->stop();
    }
}

Client::Client(uint16_t port) : port_(port), running_(true) {
    g_client_instance = this;
    sock_ = create_udp_client_socket();
    int yes = 1;
    setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
    server_len_ = sizeof(server_addr_);
}

void Client::start_discovery() {
    discovery_thread_ = thread([this]() {
        packet_t p{};
        p.type = PKT_DESC;
        p.seqn = 0;
        packet_to_network(p);

        sockaddr_in bcast{};
        bcast.sin_family = AF_INET;
        bcast.sin_port = htons(port_);
        bcast.sin_addr.s_addr = INADDR_BROADCAST;

        while (!discovered_ && running_) {
            udp_send(sock_, &p, sizeof(p), &bcast);

            packet_t r{};
            ssize_t n = udp_receive_packet(sock_, &r, sizeof(r), &server_addr_, DISCOVERY_TIMEOUT_SEC);
            
            if (n > 0) {
                packet_to_host(r);
                if (r.type == PKT_DESC_ACK) {
                    cout << timestamp_now() << " server_addr " << sockaddr_to_ipstr(server_addr_) << endl;
                    discovered_ = true;
                    break;
                }
            }

            if (!discovered_) {
                cerr << "Discovery attempt failed, retrying..." << endl;
                this_thread::sleep_for(chrono::seconds(1)); // Wait before retrying
            }
        }

        if (!discovered_) {
            cerr << "Discovery process terminated without finding a server." << endl;
            running_ = false;
        }
    });
}

void Client::start_interface() {
    interface_thread_ = thread([this]() {
        while (running_) {
            unique_lock<mutex> lock(print_mtx_);
            print_cv_.wait(lock, [this]() { return !print_queue_.empty() || !running_; });

            while (!print_queue_.empty()) {
                auto info = print_queue_.front();
                print_queue_.pop();
                lock.unlock();

                cout << timestamp_now()
                     << " server " << info.server_ip
                     << " id_req " << info.seqn
                     << " dest " << info.dest_ip
                     << " value " << info.value
                     << " new_balance " << info.new_balance << endl;

                lock.lock();
            }
        }
    });
}

void Client::start_processing() {
    processing_thread_ = thread([this]() {
        uint32_t seqn = 1;
        queue<string> request_queue;

        while (running_) {
            string line;
            if (request_queue.empty()) {
                if (!getline(cin, line)) {
                //cout << "EOF detected." << endl;
                    stop();
                    break;
                }
                if (line.empty()) continue;

                if (line.substr(0, 9) == "input.txt") {
                    string filepath = line.substr(0, 9);
                    ifstream file(filepath);
                    if (file.is_open()) {
                        string file_line;
                        while (getline(file, file_line)) {
                            request_queue.push(file_line);
                        }
                        file.close();
                        //cout << "Loaded " << request_queue.size() << " requests from file." << endl;
                        continue;
                    } else {
                        //cerr << "Failed to open file: " << filepath << endl;
                        continue;
                    }
                }
            } else {
                line = request_queue.front();
                request_queue.pop();
            }

            istringstream iss(line);
            string dest_ip; uint32_t value;
            if (!(iss >> dest_ip >> value)) continue;

            packet_t req{};
            req.type = PKT_REQ;
            req.seqn = seqn;
            in_addr_t addr = inet_addr(dest_ip.c_str());
            req.body.req.dest_addr = ntohl(addr);
            req.body.req.value = value;
            
            bool acked = false;
            while (!acked && running_) {
                packet_to_network(req);
                udp_send(sock_, &req, sizeof(req), &server_addr_);

                packet_t ack{};
                sockaddr_in src_addr;
                ssize_t m = udp_receive_packet(sock_, &ack, sizeof(ack), &src_addr, REQUEST_TIMEOUT_SEC);
                
                // change from network packet to host to print the correct numbers
                packet_to_host(req);

                if (m > 0) {
                    packet_to_host(ack);

                    //cerr << "sent packet: " << req.seqn << " " << req.body.req.dest_addr << " " << req.body.req.value << endl;
                    //cerr << "receiv packet: " << ack.seqn << " " << ack.body.req.dest_addr << endl;

                    if (ack.type != PKT_REQ_ACK) {
                        //cerr << "Invalid response type." << endl;
                        continue;
                    }

                    if (ack.seqn >= req.seqn) {
                        acked = true;
                        
                        if (ack.body.ack.error == ERR_INSUFFICIENT_FUNDS) {
                            //cerr << "Insufficient funds for transaction." << endl;
                            continue;
                        }

                        lock_guard<mutex> lock(print_mtx_);
                        print_queue_.push({sockaddr_to_ipstr(src_addr), ack.seqn, dest_ip, value, ack.body.ack.new_balance});
                        print_cv_.notify_one();
                        acked = true;
                    } else {
                        //cerr << "Received outdated ACK sequence number: " << ack.seqn << ", expected: " << req.seqn << endl;
                    }
                }
                
                if (!acked) {
                    //cerr << "Request timed out or received outdated ACK, retrying for seq " << req.seqn << endl;
                    this_thread::sleep_for(chrono::milliseconds(100)); // Shorter wait before retrying
                }
            }
            seqn++;
        }
    });
}

void Client::stop() {
    running_ = false;
    
    packet_t exit_packet{};
    exit_packet.type = PKT_EXIT;
    exit_packet.seqn = 0;
    packet_to_network(exit_packet);
    udp_send(sock_, &exit_packet, sizeof(exit_packet), &server_addr_);
    
    packet_t ack{};
    bool acked = false;
    for (int i = 0; i < 5 && !acked; ++i) {
        ssize_t n = udp_receive_packet(sock_, &ack, sizeof(ack), &server_addr_, 0.1);
        if (n > 0) {
            packet_to_host(ack);
            if (ack.type == PKT_EXIT_ACK) {
                acked = true;
                cout << "Received EXIT_ACK from server." << endl;
            }
        }
        if (!acked) {
            cerr << "Waiting for EXIT_ACK from server..." << endl;
        }
    }
    if (!acked) {
        cerr << "Did not receive EXIT_ACK from server. Forcing shutdown." << endl;
    }
}

void Client::run() {
    signal(SIGINT, signal_handler);

    start_discovery();
    discovery_thread_.join();
    if (!discovered_) return;

    start_interface();
    start_processing();

    while (running_) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    //cout << "Shutting down client..." << endl;
    processing_thread_.join();
    print_cv_.notify_all();
    interface_thread_.join();
    close(sock_);
    exit(0);
}
