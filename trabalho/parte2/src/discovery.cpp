#include "../include/discovery.hpp"
#include "../include/packet.hpp"
#include "../include/udp_util.hpp"
#include <thread>
#include <iostream>
#include <arpa/inet.h>

using namespace std;

void handle_discovery_request(int sockfd, ServerState& state, const sockaddr_in& src) {
    uint32_t ip = ntohl(src.sin_addr.s_addr);
    state.add_client(ip, ntohs(src.sin_port));
    packet_t ack{};
    ack.type = PKT_DESC_ACK;
    ack.seqn = 0;
    packet_to_network(ack);
    udp_send(sockfd, &ack, sizeof(ack), &src);
}
