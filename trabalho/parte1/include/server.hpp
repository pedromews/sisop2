#pragma once
#include "server_state.hpp"
#include "packet.hpp"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std;

struct PrintInfo {
    string client_ip;
    uint32_t seqn;
    string dest_ip;
    uint32_t value;
    string status;
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

        int sock_;
        ServerState state_;
        thread discovery_thread_;
        thread interface_thread_;

        mutex print_mtx_;
        condition_variable print_cv_;
        queue<PrintInfo> print_queue_;
};