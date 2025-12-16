#pragma once
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>

constexpr uint16_t PKT_DESC = 1;
constexpr uint16_t PKT_DESC_ACK = 2;
constexpr uint16_t PKT_REQ = 3;
constexpr uint16_t PKT_REQ_ACK = 4;
constexpr uint16_t PKT_EXIT = 5;
constexpr uint16_t PKT_EXIT_ACK = 6;
constexpr uint16_t PKT_HEARTBEAT = 7;
constexpr uint16_t PKT_HEARTBEAT_ACK = 8;
constexpr uint16_t PKT_STATE_UPDATE = 9;      // Replication
constexpr uint16_t PKT_STATE_UPDATE_ACK = 10;
constexpr uint16_t PKT_ELECTION = 11;
constexpr uint16_t PKT_ELECTION_ACK = 12;     // "I am alive" / Answer
constexpr uint16_t PKT_COORDINATOR = 13;      // Victory
constexpr uint16_t PKT_LEADER_CHANGE = 14;    // Notify client
constexpr uint16_t PKT_SNAPSHOT_REQ = 15;     // Backup asks for state
constexpr uint16_t PKT_SNAPSHOT_DATA = 16;    // Primary sends state
constexpr uint16_t PKT_SNAPSHOT_STATS = 17;   // Global stats sync
constexpr uint16_t PKT_SERVER_DESC = 18;      // Server Discovery
constexpr uint16_t PKT_SERVER_DESC_ACK = 19;  // Server Discovery ACK

#pragma pack(push,1)
struct request {
    uint32_t dest_addr;
    uint32_t value;
};

struct request_ack {
    uint32_t seqn;
    uint32_t new_balance;
    uint32_t error;
};

struct replication {
    uint32_t src_addr;
    uint16_t src_port;
    uint32_t dest_addr;
    uint32_t value;
    // seqn is in the packet header
};

struct election {
    uint32_t id; // ID of the sender
};

struct coordinator {
    uint32_t id; // ID of the new leader
};

struct snapshot_data {
    uint32_t address;
    uint16_t port;
    uint32_t last_req;
    uint32_t balance;
};

struct snapshot_stats {
    uint32_t num_trans_hi;
    uint32_t num_trans_lo;
    uint32_t total_trans_hi;
    uint32_t total_trans_lo;
};

struct server_desc {
    uint32_t id;
    uint16_t port;
};

typedef struct packet {
    uint16_t type;
    uint32_t seqn;
    union {
        request req;
        request_ack ack;
        replication repl;
        election elect;
        coordinator coord;
        snapshot_data snap;
        snapshot_stats stats;
        server_desc sdesc;
    } body;
} packet_t;
#pragma pack(pop)

// Convert packet_t fields to network byte order before sending
inline void packet_to_network(packet_t& p) {
    uint16_t t = p.type;
    
    p.type = htons(t);
    p.seqn = htonl(p.seqn);
    
    if (t == PKT_REQ) {
        p.body.req.dest_addr = htonl(p.body.req.dest_addr);
        p.body.req.value = htonl(p.body.req.value);
    } else if (t == PKT_REQ_ACK) {
        p.body.ack.seqn = htonl(p.body.ack.seqn);
        p.body.ack.new_balance = htonl(p.body.ack.new_balance);
    } else if (t == PKT_STATE_UPDATE) {
        p.body.repl.src_addr = htonl(p.body.repl.src_addr);
        p.body.repl.src_port = htons(p.body.repl.src_port);
        p.body.repl.dest_addr = htonl(p.body.repl.dest_addr);
        p.body.repl.value = htonl(p.body.repl.value);
    } else if (t == PKT_ELECTION || t == PKT_ELECTION_ACK || t == PKT_HEARTBEAT) {
        p.body.elect.id = htonl(p.body.elect.id);
    } else if (t == PKT_COORDINATOR) {
        p.body.coord.id = htonl(p.body.coord.id);
    } else if (t == PKT_SNAPSHOT_DATA) {
        p.body.snap.address = htonl(p.body.snap.address);
        p.body.snap.port = htons(p.body.snap.port);
        p.body.snap.last_req = htonl(p.body.snap.last_req);
        p.body.snap.balance = htonl(p.body.snap.balance);
    } else if (t == PKT_SNAPSHOT_STATS) {
        p.body.stats.num_trans_hi = htonl(p.body.stats.num_trans_hi);
        p.body.stats.num_trans_lo = htonl(p.body.stats.num_trans_lo);
        p.body.stats.total_trans_hi = htonl(p.body.stats.total_trans_hi);
        p.body.stats.total_trans_lo = htonl(p.body.stats.total_trans_lo);
    } else if (t == PKT_SERVER_DESC || t == PKT_SERVER_DESC_ACK) {
        p.body.sdesc.id = htonl(p.body.sdesc.id);
        p.body.sdesc.port = htons(p.body.sdesc.port);
    }
}

// Convert packet_t fields from network to host order after sending
inline void packet_to_host(packet_t& p) {
    p.type = ntohs(p.type);
    p.seqn = ntohl(p.seqn);
    
    if (p.type == PKT_REQ) {
        p.body.req.dest_addr = ntohl(p.body.req.dest_addr);
        p.body.req.value = ntohl(p.body.req.value);
    } else if (p.type == PKT_REQ_ACK) {
        p.body.ack.seqn = ntohl(p.body.ack.seqn);
        p.body.ack.new_balance = ntohl(p.body.ack.new_balance);
    } else if (p.type == PKT_STATE_UPDATE) {
        p.body.repl.src_addr = ntohl(p.body.repl.src_addr);
        p.body.repl.src_port = ntohs(p.body.repl.src_port);
        p.body.repl.dest_addr = ntohl(p.body.repl.dest_addr);
        p.body.repl.value = ntohl(p.body.repl.value);
    } else if (p.type == PKT_ELECTION || p.type == PKT_ELECTION_ACK || p.type == PKT_HEARTBEAT) {
        p.body.elect.id = ntohl(p.body.elect.id);
    } else if (p.type == PKT_COORDINATOR) {
        p.body.coord.id = ntohl(p.body.coord.id);
    } else if (p.type == PKT_SNAPSHOT_DATA) {
        p.body.snap.address = ntohl(p.body.snap.address);
        p.body.snap.port = ntohs(p.body.snap.port);
        p.body.snap.last_req = ntohl(p.body.snap.last_req);
        p.body.snap.balance = ntohl(p.body.snap.balance);
    } else if (p.type == PKT_SNAPSHOT_STATS) {
        p.body.stats.num_trans_hi = ntohl(p.body.stats.num_trans_hi);
        p.body.stats.num_trans_lo = ntohl(p.body.stats.num_trans_lo);
        p.body.stats.total_trans_hi = ntohl(p.body.stats.total_trans_hi);
        p.body.stats.total_trans_lo = ntohl(p.body.stats.total_trans_lo);
    } else if (p.type == PKT_SERVER_DESC || p.type == PKT_SERVER_DESC_ACK) {
        p.body.sdesc.id = ntohl(p.body.sdesc.id);
        p.body.sdesc.port = ntohs(p.body.sdesc.port);
    }
}
