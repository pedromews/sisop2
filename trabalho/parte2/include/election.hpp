#pragma once
#include "packet.hpp"
#include "peer.hpp"
#include <vector>
#include <chrono>

class Election {
public:
    Election(uint32_t id, std::vector<Peer> peers, int sock);

    // Starts election. Returns true if this node wins immediately (no higher peers).
    bool start_election();
    
    // Returns true if we should restart election (received from lower ID)
    bool handle_election(const packet_t& p, const sockaddr_in& src);
    
    void handle_election_ack(const packet_t& p, const sockaddr_in& src);
    
    // Returns true if election timed out and should be restarted
    bool check_timeout();
    
    bool is_in_progress() const;
    void announce_victory();
    void stop();
    bool higher_responded() const;

private:
    uint32_t id_;
    std::vector<Peer> peers_;
    int sock_;
    
    bool election_in_progress_;
    std::chrono::steady_clock::time_point election_start_time_;
    bool received_ack_;
};