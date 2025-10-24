#pragma once
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <cstdint>
#include <utility>

struct ClientEntry {
    uint32_t address;
    uint32_t last_req;
    uint32_t balance;
};

struct ServerStats {
    uint64_t num_transactions = 0;
    uint64_t total_transferred = 0;
    uint64_t total_balance = 0;
};

class ServerState {
    public:
        ServerState(uint32_t registration_balance);
        void add_client(uint32_t ip);
        bool client_exists(uint32_t ip);
        std::tuple<bool,uint32_t,uint32_t> process_req(uint32_t src_ip, uint32_t seqn, uint32_t dest_ip, uint32_t value);

        ServerStats get_stats();

    private:
        std::unordered_map<uint32_t, ClientEntry> clients_;
        ServerStats stats_;
        uint32_t registration_balance_;
        std::shared_mutex clients_mtx_;
        std::mutex stats_mtx_;
};
