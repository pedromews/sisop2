#include "discovery.hpp"
#include "packet.hpp"
#include <thread>
#include <iostream>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

using namespace std;

void discovery_server_loop(int sockfd, ServerState& state, uint32_t registration_balance) {
    while (true) {
        sockaddr_in src{};
        socklen_t slen = sizeof(src);
        packet_t p{};
        ssize_t n = recvfrom(sockfd, &p, sizeof(p), 0, (sockaddr*)&src, &slen);
        
        if (n <= 0) continue;
        
        packet_to_host(p);
        
        if (p.type == PKT_DESC) {
            uint32_t ip = ntohl(src.sin_addr.s_addr);
            state.add_client(ip);
            packet_t ack{};
            ack.type = PKT_DESC_ACK;
            ack.seqn = 0;
            packet_to_network(ack);
            sendto(sockfd, &ack, sizeof(ack), 0, (sockaddr*)&src, slen);
        }
    }
}
