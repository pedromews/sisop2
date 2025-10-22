#pragma once
#include "packet.hpp"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <netinet/in.h>

using namespace std;

struct ClientPrintInfo {
    string server_ip;
    uint32_t seqn;
    string dest_ip;
    uint32_t value;
    uint32_t new_balance;
};

class Client {
    public:
        Client(uint16_t port);
        void run();

    private:
        void start_discovery();
        void start_interface();
        void start_processing();
        bool wait_for_socket(int fd, int seconds);
        string timestamp_now();

        int sock_;
        uint16_t port_;
        sockaddr_in server_addr_;
        socklen_t server_len_;
        thread discovery_thread_;
        thread interface_thread_;
        thread processing_thread_;

        mutex print_mtx_;
        condition_variable print_cv_;
        queue<ClientPrintInfo> print_queue_;

        mutex input_mtx_;
        queue<pair<string, uint32_t>> input_queue_;

        bool discovered_ = false;
        bool running_ = true;
};
