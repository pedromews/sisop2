#include "server_state.hpp"
#include <iostream>

using namespace std;

ServerState::ServerState(uint32_t registration_balance) : registration_balance_(registration_balance) {}

void ServerState::add_client(uint32_t ip) {
    unique_lock lock(clients_mtx_);
    
    if (clients_.count(ip) == 0) {
        ClientEntry e{ip, 0, registration_balance_};
        clients_[ip] = e;

        //cerr << "add client " << ip << endl;
        
        lock.unlock();
        lock_guard s(stats_mtx_);
        
        stats_.total_balance += registration_balance_;
    }
}

// Method behavior:
// - if seqn <= last_req: we treat as duplicate -> request is not processed, reply with last_req and balance
// - if seqn > last_req + 1: there is a gap -> request is not processed, reply with last_req and balance
// - if seqn == last_req + 1: request is processed and reply with seqn and new balance
tuple<uint32_t,uint32_t,uint32_t> ServerState::process_req(uint32_t src_ip, uint32_t seqn, uint32_t dest_ip, uint32_t value) {
    unique_lock lock(clients_mtx_);
    
    auto it = clients_.find(src_ip);
    if (it == clients_.end()) {
        //cerr << "couldnt find" << endl;
        return {0, 0, ERR_CLIENT_NOT_FOUND};
    }
    //cerr << "found" << endl;
    
    uint32_t last = it->second.last_req;
    uint32_t bal = it->second.balance;
    
    if (seqn <= last) {
        // duplicate or retransmit of already processed
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
