#include "../include/server.hpp"
#include "../include/peer.hpp"
#include <iostream>
#include <vector>
#include <arpa/inet.h>

using namespace std;

#define REGISTRATION_BALANCE 10000

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: ./server <port> <id>\n";
        return 1;
    }

    try {
        uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
        uint32_t id = static_cast<uint32_t>(std::stoi(argv[2]));
        
        std::vector<Peer> peers;
        // Peers will be discovered dynamically

        Server server(port, id, peers, REGISTRATION_BALANCE);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
