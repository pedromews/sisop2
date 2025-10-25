#include "server.hpp"
#include "udp_util.hpp"
#include <iostream>
#include <stdexcept>

using namespace std;

#define REGISTRATION_BALANCE 100

int main(int argc, char** argv) {
    #ifdef _WIN32
    try {
        initialize_winsock();
    } catch (const std::runtime_error& e) {
        std::cerr << "Failed to initialize Winsock: " << e.what() << std::endl;
        return 1;
    }
    #endif
    if (argc != 2) {
        std::cerr << "Usage: ./server <port>\n";
        return 1;
    }

    uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
    try {
        Server server(port, REGISTRATION_BALANCE);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    #ifdef _WIN32
    cleanup_winsock();
    #endif

    return 0;
}
