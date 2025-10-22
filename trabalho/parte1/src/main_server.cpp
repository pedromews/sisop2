#include "server.hpp"
#include <iostream>

using namespace std;

#define REGISTRATION_BALANCE 100

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: ./server <port>\n";
        return 1;
    }

    uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
    Server server(port, REGISTRATION_BALANCE);
    server.run();

    return 0;
}
