#include "udp_util.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

using namespace std;

int create_udp_server_socket(uint16_t port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sock < 0) throw runtime_error("socket() failed");
    
    int yes = 1;
    
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        close(sock);
        throw runtime_error("setsockopt failed");
    }
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (::bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        throw runtime_error("bind() failed");
    }
    
    return sock;
}

int create_udp_client_socket() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw runtime_error("socket() failed");
    return sock;
}

string sockaddr_to_ipstr(const sockaddr_in& sa) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sa.sin_addr, buf, sizeof(buf));
    return string(buf);
}
