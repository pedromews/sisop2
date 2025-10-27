#include "server.hpp"
#include "udp_util.hpp"
#include "discovery.hpp"
#include "utils.hpp"
#include <iostream>
#include <arpa/inet.h>

using namespace std;

Server::Server(uint16_t port, uint32_t registration_balance)
    : sock_(create_udp_server_socket(port)), state_(registration_balance) {
    cout << timestamp_now()
        << " num_transactions 0 total_transferred 0 total_balance 0" << endl;
}

void Server::run() {
    start_discovery();
    start_interface();

    while (true) {
        sockaddr_in src{};
        packet_t p{};
        ssize_t n = udp_receive(sock_, &p, sizeof(p), &src);
        
        if (n <= 0) continue;

        packet_to_host(p);

        if (p.type == PKT_REQ) {
            thread worker(&Server::handle_request, this, p, src);
            worker.detach();
        } else if (p.type == PKT_EXIT) {
            handle_exit(src);
        }
    }

    discovery_thread_.join();
    interface_thread_.join();
}

void Server::handle_exit(const sockaddr_in& src) {
    uint32_t client_ip = ntohl(src.sin_addr.s_addr);
    string client_ip_str = sockaddr_to_ipstr(src);

    try {
        state_.remove_client(client_ip);
        //cout << timestamp_now() << " Client " << client_ip_str << " has exited." << endl;
        
        auto stats = state_.get_stats();
        // cout << "Updated stats - num_transactions: " << stats.num_transactions
        //      << ", total_transferred: " << stats.total_transferred
        //      << ", total_balance: " << stats.total_balance << endl;
    } catch (const std::exception& e) {
        //cerr << timestamp_now() << " Error handling exit for client " << client_ip_str 
        //     << ": " << e.what() << endl;
    }

    // Send EXIT_ACK to client
    packet_t ack{};
    ack.type = PKT_EXIT_ACK;
    ack.seqn = 0;
    packet_to_network(ack);
    udp_send(sock_, &ack, sizeof(ack), &src);
}

void Server::start_discovery() {
    discovery_thread_ = thread([this]() {
        discovery_server_loop(sock_, state_, 100);
    });
}

void Server::start_interface() {
    interface_thread_ = thread([this]() {
        while (true) {
            unique_lock<mutex> lock(print_mtx_);
            print_cv_.wait(lock, [this]() { return !print_queue_.empty(); });

            while (!print_queue_.empty()) {
                auto info = print_queue_.front();
                print_queue_.pop();
                lock.unlock();

                if (info.status != "OK" && info.status != "DUP")
                    continue;

                string status_str = (info.status != "OK")
                                    ? (string(" ") + info.status + "!")
                                    : string();

                cout << timestamp_now()
                     << " client " << info.client_ip
                     << status_str
                     << " id_req " << info.seqn
                     << " dest " << info.dest_ip
                     << " value " << info.value
                     << " num_transactions " << info.stats.num_transactions
                     << " total_transferred " << info.stats.total_transferred
                     << " total_balance " << info.stats.total_balance << endl;

                lock.lock();
            }
        }
    });
}

void Server::handle_request(packet_t p, sockaddr_in src) {
    uint32_t src_ip = ntohl(src.sin_addr.s_addr);
    auto [ack_seq, balance, error] = state_.process_req(src_ip, p.seqn, p.body.req.dest_addr, p.body.req.value);

    //cerr << "error = " << error << endl;

    packet_t ack{};
    ack.type = PKT_REQ_ACK;
    ack.seqn = ack_seq;
    ack.body.ack.seqn = ack_seq;
    ack.body.ack.new_balance = balance;
    ack.body.ack.error = error;
    packet_to_network(ack);
    udp_send(sock_, &ack, sizeof(ack), &src);

    string client_ip = sockaddr_to_ipstr(src);
    in_addr dest_in; dest_in.s_addr = htonl(p.body.req.dest_addr);
    string dest_ip = inet_ntoa(dest_in);
    string status;

    switch (error) {
        case ERR_CLIENT_NOT_FOUND:
            status = "CLIENT NOT FOUND";
            break;
        case ERR_DUPLICATE_REQ:
            status = "DUP";
            break;
        case ERR_MISSING_PREV_REQ:
            status = "MISSING";
            break;
        case ERR_INSUFFICIENT_FUNDS:
            status = "NO FUNDS";
            break;
        default:
            status = "OK";
            break;
    }

    //cerr << "status = " << status << endl;

    auto stats = state_.get_stats();
    {
        lock_guard<mutex> lock(print_mtx_);
        print_queue_.push({client_ip, p.seqn, dest_ip, p.body.req.value, status, stats});
    }
    print_cv_.notify_one();
}
