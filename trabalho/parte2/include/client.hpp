#pragma once
#include "packet.hpp"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <netinet/in.h>
#include <atomic>
#include <chrono>

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
        void stop();

    private:
        void find_server();
        void start_interface();
        void start_processing();

        int sock_;
        uint16_t port_;
        sockaddr_in server_addr_;
        socklen_t server_len_;
        std::thread interface_thread_;
        std::thread processing_thread_;

        std::mutex print_mtx_;
        std::condition_variable print_cv_;
        std::queue<ClientPrintInfo> print_queue_;

        std::mutex input_mtx_;
        std::queue<std::pair<std::string, uint32_t>> input_queue_;

        std::atomic<bool> discovered_{false};
        std::atomic<bool> running_{true};

        static constexpr double DISCOVERY_TIMEOUT_SEC = 0.5;
        static constexpr double REQUEST_TIMEOUT_SEC = 0.01; // 10 milliseconds
};
