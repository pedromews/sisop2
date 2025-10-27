#include "server.hpp"
#include <iostream>

using namespace std;

#define REGISTRATION_BALANCE 100

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: ./server <port>\n";
        return 1;
    }

    try {
        uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
        Server server(port, REGISTRATION_BALANCE);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
