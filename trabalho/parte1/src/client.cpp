#include "client.hpp"
#include "udp_util.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

using namespace std;

constexpr int CLIENT_RETRIES = 5;
constexpr int CLIENT_TIMEOUT_SEC = 2;

Client::Client(uint16_t port) : port_(port) {
    sock_ = create_udp_client_socket();
    int yes = 1;
    setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
    server_len_ = sizeof(server_addr_);
}

bool Client::wait_for_socket(int fd, int seconds) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;

    int rv = select(fd + 1, &readfds, NULL, NULL, &tv);
    return rv > 0 && FD_ISSET(fd, &readfds);
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

        for (int attempt = 0; attempt < CLIENT_RETRIES && !discovered_; ++attempt) {
            sendto(sock_, &p, sizeof(p), 0, (sockaddr*)&bcast, sizeof(bcast));

            if (wait_for_socket(sock_, CLIENT_TIMEOUT_SEC)) {
                packet_t r{};
                ssize_t n = recvfrom(sock_, &r, sizeof(r), 0, (sockaddr*)&server_addr_, &server_len_);
                if (n > 0) {
                    packet_to_host(r);
                    if (r.type == PKT_DESC_ACK) {
                        cout << "server addr " << inet_ntoa(server_addr_.sin_addr) << endl;
                        discovered_ = true;
                        break;
                    }
                }
            }
        }

        if (!discovered_) {
            cerr << "Discovery failed after retries";
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

                cout << "server " << info.server_ip
                     << " id req " << info.seqn
                     << " dest " << info.dest_ip
                     << " value " << info.value
                     << " new balance " << info.new_balance << endl;

                lock.lock();
            }
        }
    });
}

void Client::start_processing() {
    processing_thread_ = thread([this]() {
        uint32_t seqn = 1;
        while (running_) {
            string line;
            if (!getline(cin, line)) break;
            if (line.empty()) continue;

            istringstream iss(line);
            string dest_ip; uint32_t value;
            if (!(iss >> dest_ip >> value)) continue;

            packet_t req{};
            req.type = PKT_REQ;
            req.seqn = seqn;
            in_addr_t addr = inet_addr(dest_ip.c_str());
            req.body.req.dest_addr = ntohl(addr);
            req.body.req.value = value;
            packet_to_network(req);

            bool acked = false;
            for (int attempt = 0; attempt < CLIENT_RETRIES && !acked; ++attempt) {
                sendto(sock_, &req, sizeof(req), 0, (sockaddr*)&server_addr_, server_len_);

                if (wait_for_socket(sock_, CLIENT_TIMEOUT_SEC)) {
                    packet_t ack{};
                    ssize_t m = recvfrom(sock_, &ack, sizeof(ack), 0, NULL, NULL);
                    if (m > 0) {
                        packet_to_host(ack);
                        if (ack.type == PKT_REQ_ACK) {
                            {
                                lock_guard<mutex> lock(print_mtx_);
                                print_queue_.push({inet_ntoa(server_addr_.sin_addr), ack.seqn, dest_ip, value, ack.body.ack.new_balance});
                            }
                            print_cv_.notify_one();
                            acked = true;
                            break;
                        }
                    }
                }
            }
            if (!acked) {
                cerr << "Request timed out after retries for seq " << seqn << "";
            }
            seqn++;
        }
        running_ = false;
    });
}

void Client::run() {
    start_discovery();
    discovery_thread_.join();
    if (!discovered_) return;

    start_interface();
    start_processing();

    processing_thread_.join();
    running_ = false;
    print_cv_.notify_all();
    interface_thread_.join();
    close(sock_);
}
