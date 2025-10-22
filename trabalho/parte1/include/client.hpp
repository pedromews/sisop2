#pragma once
#include "packet.hpp"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <netinet/in.h>

struct ClientPrintInfo {
    std::string server_ip;
    uint32_t seqn;
    std::string dest_ip;
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

        int sock_;
        uint16_t port_;
        sockaddr_in server_addr_;
        socklen_t server_len_;
        std::thread discovery_thread_;
        std::thread interface_thread_;
        std::thread processing_thread_;

        std::mutex print_mtx_;
        std::condition_variable print_cv_;
        std::queue<ClientPrintInfo> print_queue_;

        std::mutex input_mtx_;
        std::queue<std::pair<std::string, uint32_t>> input_queue_;

        bool discovered_ = false;
        bool running_ = true;
};
