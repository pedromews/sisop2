#pragma once
#include "server_state.hpp"
#include "packet.hpp"
#include "peer.hpp"
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>

class ReplicaManager {
public:
    ReplicaManager(uint32_t id, std::vector<Peer> peers, int sock, ServerState& state);

    // Role management
    bool is_primary() const;
    void set_primary(bool is_primary);
    std::atomic<bool>& get_primary_flag() { return is_primary_; }
    
    uint32_t get_leader_id() const;
    void set_leader_id(uint32_t id);

    // Heartbeats
    void send_heartbeats();
    void on_heartbeat_received(uint32_t sender_id);
    bool check_heartbeat_timeout(); // Returns true if timeout occurred

    // Replication
    void replicate_request(const packet_t& original_req, uint32_t src_ip, uint16_t src_port);
    void handle_replication(const packet_t& p);

    // Snapshot
    void request_snapshot();
    void handle_snapshot_req(const sockaddr_in& src);
    void handle_snapshot_data(const packet_t& p);
    void send_snapshot_to_leader();
    void handle_snapshot_stats(const packet_t& p);
    
    void add_peer(const Peer& peer);

private:
    uint32_t id_;
    std::vector<Peer> peers_;
    int sock_;
    ServerState& state_;

    std::atomic<bool> is_primary_;
    uint32_t leader_id_;
    
    std::chrono::steady_clock::time_point last_heartbeat_received_;
    std::mutex peers_mtx_;
};