#include "../include/replica_manager.hpp"
#include "../include/udp_util.hpp"
#include "../include/utils.hpp"
#include <iostream>

using namespace std;

ReplicaManager::ReplicaManager(uint32_t id, std::vector<Peer> peers, int sock, ServerState& state)
    : id_(id), peers_(peers), sock_(sock), state_(state), is_primary_(false), leader_id_(0) {
    last_heartbeat_received_ = std::chrono::steady_clock::now();
}

bool ReplicaManager::is_primary() const {
    return is_primary_;
}

void ReplicaManager::set_primary(bool is_primary) {
    is_primary_ = is_primary;
}

uint32_t ReplicaManager::get_leader_id() const {
    return leader_id_;
}

void ReplicaManager::set_leader_id(uint32_t id) {
    leader_id_ = id;
}

void ReplicaManager::send_heartbeats() {
    packet_t p{};
    p.type = PKT_HEARTBEAT;
    p.body.elect.id = id_;
    packet_to_network(p);
    
    std::lock_guard<std::mutex> lock(peers_mtx_);
    for (const auto& peer : peers_) {
        udp_send(sock_, &p, sizeof(p), &peer.addr);
    }
}

void ReplicaManager::on_heartbeat_received(uint32_t sender_id) {
    if (sender_id == leader_id_ || leader_id_ == 0) {
        last_heartbeat_received_ = std::chrono::steady_clock::now();
        if (leader_id_ == 0) {
            leader_id_ = sender_id;
        }
    }
}

bool ReplicaManager::check_heartbeat_timeout() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_heartbeat_received_).count();
    // Timeout: 3 seconds
    return (duration > 3000 && leader_id_ != id_);
}

void ReplicaManager::replicate_request(const packet_t& original_req, uint32_t src_ip, uint16_t src_port) {
    packet_t repl{};
    repl.type = PKT_STATE_UPDATE;
    repl.seqn = original_req.seqn;
    repl.body.repl.src_addr = src_ip;
    repl.body.repl.src_port = src_port;
    repl.body.repl.dest_addr = original_req.body.req.dest_addr;
    repl.body.repl.value = original_req.body.req.value;
    
    packet_to_network(repl);
    std::lock_guard<std::mutex> lock(peers_mtx_);
    for (const auto& peer : peers_) {
        udp_send(sock_, &repl, sizeof(repl), &peer.addr);
    }
}

void ReplicaManager::handle_replication(const packet_t& p) {
    state_.apply_replication(p.body.repl.src_addr, p.body.repl.src_port, p.body.repl.dest_addr, p.body.repl.value, p.seqn);
}

void ReplicaManager::request_snapshot() {
    packet_t p{};
    p.type = PKT_SNAPSHOT_REQ;
    packet_to_network(p);
    
    std::lock_guard<std::mutex> lock(peers_mtx_);
    for (const auto& peer : peers_) {
        if (peer.id == leader_id_) {
            udp_send(sock_, &p, sizeof(p), &peer.addr);
            break;
        }
    }
}

void ReplicaManager::handle_snapshot_req(const sockaddr_in& src) {
    auto clients = state_.get_all_clients();
    auto stats = state_.get_stats();
    
    packet_t ps{};
    ps.type = PKT_SNAPSHOT_STATS;
    ps.body.stats.num_trans_hi = stats.num_transactions >> 32;
    ps.body.stats.num_trans_lo = stats.num_transactions & 0xFFFFFFFF;
    ps.body.stats.total_trans_hi = stats.total_transferred >> 32;
    ps.body.stats.total_trans_lo = stats.total_transferred & 0xFFFFFFFF;
    packet_to_network(ps);
    udp_send(sock_, &ps, sizeof(ps), &src);

    for (const auto& c : clients) {
        packet_t p{};
        p.type = PKT_SNAPSHOT_DATA;
        p.body.snap.address = c.address;
        p.body.snap.port = c.port;
        p.body.snap.last_req = c.last_req;
        p.body.snap.balance = c.balance;
        packet_to_network(p);
        udp_send(sock_, &p, sizeof(p), &src);
    }
}

void ReplicaManager::handle_snapshot_data(const packet_t& p) {
    state_.update_client_absolute(p.body.snap.address, p.body.snap.port, p.body.snap.balance, p.body.snap.last_req);
}

void ReplicaManager::send_snapshot_to_leader() {
    auto stats = state_.get_stats();
    packet_t ps{};
    ps.type = PKT_SNAPSHOT_STATS;
    ps.body.stats.num_trans_hi = stats.num_transactions >> 32;
    ps.body.stats.num_trans_lo = stats.num_transactions & 0xFFFFFFFF;
    ps.body.stats.total_trans_hi = stats.total_transferred >> 32;
    ps.body.stats.total_trans_lo = stats.total_transferred & 0xFFFFFFFF;
    packet_to_network(ps);

    auto clients = state_.get_all_clients();
    for (const auto& c : clients) {
        packet_t p{};
        p.type = PKT_SNAPSHOT_DATA;
        p.body.snap.address = c.address;
        p.body.snap.port = c.port;
        p.body.snap.last_req = c.last_req;
        p.body.snap.balance = c.balance;
        packet_to_network(p);
        std::lock_guard<std::mutex> lock(peers_mtx_);
        for (const auto& peer : peers_) {
            if (peer.id == leader_id_) {
                udp_send(sock_, &p, sizeof(p), &peer.addr);
                udp_send(sock_, &ps, sizeof(ps), &peer.addr); // Send stats as well
                break;
            }
        }
    }
}

void ReplicaManager::handle_snapshot_stats(const packet_t& p) {
    uint64_t num = ((uint64_t)p.body.stats.num_trans_hi << 32) | p.body.stats.num_trans_lo;
    uint64_t total = ((uint64_t)p.body.stats.total_trans_hi << 32) | p.body.stats.total_trans_lo;
    state_.update_global_stats(num, total);
}

void ReplicaManager::add_peer(const Peer& peer) {
    std::lock_guard<std::mutex> lock(peers_mtx_);
    for (const auto& p : peers_) {
        if (p.id == peer.id) return;
    }
    peers_.push_back(peer);
    // cout << "ReplicaManager added peer " << peer.id << endl;
}