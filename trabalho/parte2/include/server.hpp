#pragma once
#include "server_state.hpp"
#include "packet.hpp"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

struct PrintInfo {
    std::string client_ip;
    uint32_t seqn;
    std::string dest_ip;
    uint32_t value;
    std::string status;
    ServerStats stats;
};

class Server {
    public:
        Server(uint16_t port, uint32_t registration_balance);
        void run();

    private:
        void start_discovery();
        void start_interface();
        void handle_request(packet_t p, sockaddr_in src);
        void handle_exit(const sockaddr_in& src);

        int sock_;
        ServerState state_;
        std::thread discovery_thread_;
        std::thread interface_thread_;

        std::mutex print_mtx_;
        std::condition_variable print_cv_;
        std::queue<PrintInfo> print_queue_;
};
