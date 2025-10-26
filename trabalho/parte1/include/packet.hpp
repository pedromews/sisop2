#pragma once
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>

constexpr uint16_t PKT_DESC = 1;
constexpr uint16_t PKT_DESC_ACK = 2;
constexpr uint16_t PKT_REQ = 3;
constexpr uint16_t PKT_REQ_ACK = 4;

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

typedef struct packet {
    uint16_t type;
    uint32_t seqn;
    union {
        request req;
        request_ack ack;
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
    }
}
