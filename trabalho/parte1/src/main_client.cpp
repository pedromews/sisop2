#include "client.hpp"
#include "udp_util.hpp"
#include <iostream>
#include <stdexcept>

using namespace std;

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
        cerr << "Usage: ./cliente <port>" << endl;
        return 1;
    }

    uint16_t port = static_cast<uint16_t>(stoi(argv[1]));
    
    try {
        Client client(port);
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    #ifdef _WIN32
    cleanup_winsock();
    #endif

    return 0;
}
