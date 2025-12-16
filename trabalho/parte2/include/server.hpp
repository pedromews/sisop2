#pragma once
#include "server_state.hpp"
#include "packet.hpp"
#include "peer.hpp"
#include "replica_manager.hpp"
#include "election.hpp"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>

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
        Server(uint16_t port, uint32_t id, std::vector<Peer> peers, uint32_t registration_balance);
        void run();

    private:
        void start_interface();
        void handle_request(packet_t p, sockaddr_in src);
        void handle_exit(const sockaddr_in& src);
        void process_packet(packet_t p, sockaddr_in src);
        
        // Part 2 delegation
        void handle_coordinator(packet_t p, sockaddr_in src);
        void become_primary();
        void notify_clients_leader_change();
        void broadcast_presence();
        
        int sock_;
        uint32_t id_;
        std::vector<Peer> peers_;
        
        ServerState state_;
        ReplicaManager replica_manager_;
        Election election_;
        
        std::thread interface_thread_;

        std::mutex print_mtx_;
        std::condition_variable print_cv_;
        std::queue<PrintInfo> print_queue_;
};
