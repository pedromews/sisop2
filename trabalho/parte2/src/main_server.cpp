#include "../include/server.hpp"
#include "../include/peer.hpp"
#include <iostream>
#include <vector>
#include <arpa/inet.h>

using namespace std;

#define REGISTRATION_BALANCE 10000

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: ./server <port> <id> [<peer_id> <peer_ip> <peer_port> ...]\n";
        return 1;
    }

    try {
        uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
        uint32_t id = static_cast<uint32_t>(std::stoi(argv[2]));
        
        std::vector<Peer> peers;
        int i = 3;
        while (i < argc) {
            if (i + 2 >= argc) {
                cerr << "Invalid peer arguments. Expected: id ip port" << endl;
                break;
            }
            uint32_t pid = static_cast<uint32_t>(stoi(argv[i++]));
            string pip = argv[i++];
            uint16_t pport = static_cast<uint16_t>(stoi(argv[i++]));
            
            sockaddr_in paddr{};
            paddr.sin_family = AF_INET;
            paddr.sin_port = htons(pport);
            paddr.sin_addr.s_addr = inet_addr(pip.c_str());
            
            peers.push_back({pid, paddr});
        }

        Server server(port, id, peers, REGISTRATION_BALANCE);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
