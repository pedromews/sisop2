#include "server.hpp"
#include "udp_util.hpp"
#include "discovery.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <chrono>
#include <ctime>
#include <iomanip>

using namespace std;

static string timestamp_now() {
    using namespace std::chrono;
    auto now = system_clock::now();
    time_t t = system_clock::to_time_t(now);
    tm tm = *localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return string(buf);
}

Server::Server(uint16_t port, uint32_t registration_balance)
    : sock_(create_udp_server_socket(port)), state_(registration_balance) {}

void Server::run() {
    start_discovery();
    start_interface();

    while (true) {
        sockaddr_in src{};
        socklen_t slen = sizeof(src);
        packet_t p{};
        ssize_t n = recvfrom(sock_, &p, sizeof(p), 0, (sockaddr*)&src, &slen);
        
        if (n <= 0) continue;

        packet_to_host(p);

        if (p.type == PKT_REQ) {
            thread worker(&Server::handle_request, this, p, src);
            worker.detach();
        }
    }

    discovery_thread_.join();
    interface_thread_.join();
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

                string status_str = (info.status != "OK")
                                    ? (string(" ") + info.status + "! ")
                                    : string();

                cout << timestamp_now()
                     << " client " << info.client_ip
                     << status_str
                     << " id_req " << info.seqn
                     << " dest " << info.dest_ip
                     << " value " << info.value << endl;

                cout << "num_transactions " << info.stats.num_transactions << endl
                     << " total_transferred " << info.stats.total_transferred
                     << " total_balance " << info.stats.total_balance << endl;

                lock.lock();
            }
        }
    });
}

void Server::handle_request(packet_t p, sockaddr_in src) {
    uint32_t src_ip = ntohl(src.sin_addr.s_addr);
    auto [processed, ack_seq, balance] = state_.process_req(src_ip, p.seqn, p.body.req.dest_addr, p.body.req.value);

    packet_t ack{};
    ack.type = PKT_REQ_ACK;
    ack.seqn = ack_seq;
    ack.body.ack.seqn = ack_seq;
    ack.body.ack.new_balance = balance;
    packet_to_network(ack);
    sendto(sock_, &ack, sizeof(ack), 0, (sockaddr*)&src, sizeof(src));

    string client_ip = inet_ntoa(src.sin_addr);
    in_addr dest_in; dest_in.s_addr = htonl(p.body.req.dest_addr);
    string dest_ip = inet_ntoa(dest_in);
    string status = processed ? "OK" : (p.seqn <= ack_seq ? "DUP" : "MISSING");

    auto stats = state_.get_stats();
    {
        lock_guard<mutex> lock(print_mtx_);
        print_queue_.push({client_ip, p.seqn, dest_ip, p.body.req.value, status, stats});
    }
    print_cv_.notify_one();
}
