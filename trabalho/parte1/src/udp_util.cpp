#include "udp_util.hpp"
#include <cstring>
#include <stdexcept>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <unistd.h>
#endif

using namespace std;

#ifdef _WIN32
void initialize_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw runtime_error("WSAStartup failed");
    }
}

void cleanup_winsock() {
    WSACleanup();
}
#endif

int create_udp_server_socket(uint16_t port) {
    #ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) throw runtime_error("socket() failed");
    #else
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw runtime_error("socket() failed");
    #endif
    
    #ifdef _WIN32
    char yes = 1;
    #else
    int yes = 1;
    #endif
    
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        #ifdef _WIN32
        closesocket(sock);
        #else
        close(sock);
        #endif
        throw runtime_error("setsockopt failed");
    }
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (::bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        #ifdef _WIN32
        closesocket(sock);
        #else
        close(sock);
        #endif
        throw runtime_error("bind() failed");
    }
    
    return sock;
}

int create_udp_client_socket() {
    #ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) throw runtime_error("socket() failed");
    #else
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw runtime_error("socket() failed");
    #endif
    return sock;
}

string sockaddr_to_ipstr(const sockaddr_in& sa) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sa.sin_addr, buf, sizeof(buf));
    return string(buf);
}

ssize_t udp_send(int sock, const void* buf, size_t len, const sockaddr_in* dest_addr) {
    #ifdef _WIN32
    return sendto(sock, static_cast<const char*>(buf), static_cast<int>(len), 0, (const sockaddr*)dest_addr, sizeof(sockaddr_in));
    #else
    return sendto(sock, buf, len, 0, (const sockaddr*)dest_addr, sizeof(sockaddr_in));
    #endif
}

ssize_t udp_receive(int sock, void* buf, size_t len, sockaddr_in* src_addr) {
    socklen_t src_len = sizeof(sockaddr_in);
    #ifdef _WIN32
    return recvfrom(sock, static_cast<char*>(buf), static_cast<int>(len), 0, (sockaddr*)src_addr, &src_len);
    #else
    return recvfrom(sock, buf, len, 0, (sockaddr*)src_addr, &src_len);
    #endif
}

bool udp_wait_for_socket(int fd, double timeout) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    struct timeval tv;
    tv.tv_sec = static_cast<long>(timeout);
    tv.tv_usec = static_cast<long>((timeout - tv.tv_sec) * 1000000);

    #ifdef _WIN32
    int rv = select(0, &readfds, NULL, NULL, &tv);
    #else
    int rv = select(fd + 1, &readfds, NULL, NULL, &tv);
    #endif
    return rv > 0 && FD_ISSET(fd, &readfds);
}

ssize_t udp_receive_packet(int sock, void* packet, size_t packet_size, sockaddr_in* src_addr, double timeout) {
    if (udp_wait_for_socket(sock, timeout)) {
        return udp_receive(sock, packet, packet_size, src_addr);
    }
    return -1; // Timeout or error
}
