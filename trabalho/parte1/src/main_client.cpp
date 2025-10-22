#include "client.hpp"
#include <iostream>

using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: ./cliente <port>" << endl;
        return 1;
    }

    uint16_t port = static_cast<uint16_t>(stoi(argv[1]));
    Client client(port);
    client.run();

    return 0;
}
