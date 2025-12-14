#include "../include/election.hpp"
#include "../include/udp_util.hpp"
#include "../include/utils.hpp"
#include <iostream>

using namespace std;

Election::Election(uint32_t id, std::vector<Peer> peers, int sock)
    : id_(id), peers_(peers), sock_(sock), election_in_progress_(false), received_ack_(false) {}

bool Election::start_election() {
    //cout << timestamp_now() << " [Election] Starting election. My ID: " << id_ << endl;
    election_in_progress_ = true;
    received_ack_ = false;
    election_start_time_ = std::chrono::steady_clock::now();
    bool higher_exists = false;
    
    packet_t p{};
    p.type = PKT_ELECTION;
    p.body.elect.id = id_;
    packet_to_network(p);

    for (const auto& peer : peers_) {
        if (peer.id > id_) {
            //cout << timestamp_now() << " [Election] Sending ELECTION to peer " << peer.id << endl;
            udp_send(sock_, &p, sizeof(p), &peer.addr);
            higher_exists = true;
        }
    }

    return !higher_exists;
}

bool Election::handle_election(const packet_t& p, const sockaddr_in& src) {
    uint32_t sender_id = p.body.elect.id;
    //cout << timestamp_now() << " [Election] Handling ELECTION from " << sender_id << endl;
    if (sender_id < id_) {
        //cout << timestamp_now() << " [Election] Sender ID " << sender_id << " < My ID " << id_ << ". Sending ACK." << endl;
        packet_t ack{};
        ack.type = PKT_ELECTION_ACK;
        ack.body.elect.id = id_;
        packet_to_network(ack);
        udp_send(sock_, &ack, sizeof(ack), &src);
        
        if (!election_in_progress_) {
            return true; // Should start election
        }
    } else {
        //cout << timestamp_now() << " [Election] Sender ID " << sender_id << " > My ID " << id_ << ". Ignoring (waiting for Coordinator)." << endl;
    }
    return false;
}

void Election::handle_election_ack(const packet_t& p, const sockaddr_in& src) {
    // Received answer from higher ID. Wait for coordinator message.
    //cout << timestamp_now() << " [Election] Received ELECTION_ACK from " << p.body.elect.id << ". Waiting for Coordinator." << endl;
    if (p.body.elect.id > id_) {
        received_ack_ = true;
    }
}

bool Election::check_timeout() {
    if (election_in_progress_) {
        auto now = std::chrono::steady_clock::now();
        auto election_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - election_start_time_).count();
        return (election_duration > 5000);
    }
    return false;
}

bool Election::is_in_progress() const {
    return election_in_progress_;
}

void Election::announce_victory() {
    //cout << timestamp_now() << " [Election] Announcing victory (COORDINATOR) to peers." << endl;
    election_in_progress_ = false;
    packet_t p{};
    p.type = PKT_COORDINATOR;
    p.body.coord.id = id_;
    packet_to_network(p);
    
    for (const auto& peer : peers_) {
        udp_send(sock_, &p, sizeof(p), &peer.addr);
    }
}

void Election::stop() {
    if (election_in_progress_) {
        //cout << timestamp_now() << " [Election] Stopping election process." << endl;
    }
    election_in_progress_ = false;
}

bool Election::higher_responded() const {
    return received_ack_;
}