#include "../include/server_state.hpp"
#include <iostream>

using namespace std;

ServerState::ServerState(uint32_t registration_balance) : registration_balance_(registration_balance) {}

void ServerState::add_client(uint32_t ip, uint16_t port) {
    unique_lock lock(clients_mtx_);
    
    if (clients_.find(ip) != clients_.end()) {
        return;
    }

    ClientEntry e{ip, port, 0, registration_balance_};
    clients_[ip] = e;
    
    lock.unlock();
    lock_guard s(stats_mtx_);
    
    stats_.total_balance += registration_balance_;
}

// Method behavior:
// - if seqn <= last_req: we treat as duplicate -> request is not processed, reply with last_req and balance
// - if seqn > last_req + 1: there is a gap -> request is not processed, reply with last_req and balance
// - if seqn == last_req + 1: request is processed and reply with seqn and new balance
tuple<uint32_t,uint32_t,uint32_t> ServerState::process_req(uint32_t src_ip, uint16_t src_port, uint32_t seqn, uint32_t dest_ip, uint32_t value) {
    unique_lock lock(clients_mtx_);
    
    auto it = clients_.find(src_ip);
    if (it == clients_.end()) {
        //cerr << "couldnt find" << endl;
        return {0, 0, ERR_CLIENT_NOT_FOUND};
    }
    //cerr << "found" << endl;
    
    // Update port in case it changed (e.g. client restart)
    it->second.port = src_port;
    
    uint32_t last = it->second.last_req;
    uint32_t bal = it->second.balance;
    
    if (seqn <= last) {
        // duplicate or retransmit of already processed
        // cout << "DEBUG: Duplicate req. seqn=" << seqn << " last=" << last << endl;
        return {last, bal, ERR_DUPLICATE_REQ};
    }
    if (seqn > last + 1) {
        // missing prior requests; do not process
        return {last, bal, ERR_MISSING_PREV_REQ};
    }
    // seqn == last + 1 -> attempt to process
    if (value > it->second.balance) {
        // insufficient funds;
        // still update last_req?
        // Decision: update last_req to seqn to avoid replay of same failed request
        it->second.last_req = seqn;
        return {seqn, it->second.balance, ERR_INSUFFICIENT_FUNDS};
    }
    
    it->second.last_req = seqn;
    
    auto it_dest = clients_.find(dest_ip);
    if (it_dest != clients_.end()) {
        it->second.balance -= value;
        it_dest->second.balance += value;
    }
    
    {
        lock_guard g(stats_mtx_);
        stats_.num_transactions++;
        stats_.total_transferred += value;
    }
    
    return {seqn, it->second.balance, NO_ERROR};
}

void ServerState::apply_replication(uint32_t src_ip, uint16_t src_port, uint32_t dest_ip, uint32_t value, uint32_t seqn) {
    unique_lock lock(clients_mtx_);
    
    uint64_t balance_increase = 0;

    // Ensure clients exist (in case Discovery packet was lost/reordered, though unlikely in this flow)
    if (clients_.find(src_ip) == clients_.end()) {
        clients_[src_ip] = {src_ip, src_port, 0, registration_balance_};
        balance_increase += registration_balance_;
        // Note: This might be inaccurate if the client had history, 
        // but in passive replication, we assume we are in sync.
    }
    if (clients_.find(dest_ip) == clients_.end()) {
        clients_[dest_ip] = {dest_ip, 0, 0, registration_balance_};
        balance_increase += registration_balance_;
    }

    ClientEntry& src = clients_[src_ip];
    ClientEntry& dest = clients_[dest_ip];

    if (seqn <= src.last_req) {
        return;
    }

    // Apply changes blindly as Primary has already validated
    src.balance -= value;
    dest.balance += value;
    src.last_req = seqn;
    src.port = src_port;

    lock.unlock();

    {
        lock_guard g(stats_mtx_);
        stats_.num_transactions++;
        stats_.total_transferred += value;
        stats_.total_balance += balance_increase;
    }
}

void ServerState::update_client_absolute(uint32_t addr, uint16_t port, uint32_t balance, uint32_t last_req) {
    unique_lock lock(clients_mtx_);
    
    uint32_t old_balance = 0;
    bool exists = false;
    if (clients_.find(addr) != clients_.end()) {
        if (clients_[addr].last_req > last_req) {
            // Local state is fresher than snapshot. Ignore.
            return;
        }
        old_balance = clients_[addr].balance;
        exists = true;
    }

    // This overwrites existing state or creates new
    clients_[addr] = {addr, port, last_req, balance};
    
    lock.unlock();
    lock_guard g(stats_mtx_);
    if (exists) stats_.total_balance -= old_balance;
    stats_.total_balance += balance;
}

void ServerState::update_global_stats(uint64_t num_transactions, uint64_t total_transferred) {
    lock_guard g(stats_mtx_);
    if (num_transactions > stats_.num_transactions) stats_.num_transactions = num_transactions;
    if (total_transferred > stats_.total_transferred) stats_.total_transferred = total_transferred;
}

ServerStats ServerState::get_stats() {
    lock_guard g(stats_mtx_);
    return stats_;
}

void ServerState::remove_client(uint32_t ip) {
    unique_lock lock(clients_mtx_);
    auto it = clients_.find(ip);
    if (it != clients_.end()) {
        uint32_t client_balance = it->second.balance;
        clients_.erase(it);
        lock.unlock();

        lock_guard g(stats_mtx_);
        stats_.total_balance -= client_balance;
    }
}

std::vector<ClientEntry> ServerState::get_all_clients() {
    std::shared_lock lock(clients_mtx_);
    std::vector<ClientEntry> result;
    result.reserve(clients_.size());
    for (const auto& kv : clients_) {
        result.push_back(kv.second);
    }
    return result;
}

void ServerState::restore_state(const std::vector<ClientEntry>& clients, const ServerStats& stats) {
    unique_lock lock(clients_mtx_);
    clients_.clear();
    for (const auto& c : clients) {
        clients_[c.address] = c;
    }
    lock.unlock();
    
    lock_guard g(stats_mtx_);
    stats_ = stats;
}
