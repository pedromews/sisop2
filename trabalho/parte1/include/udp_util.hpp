#pragma once
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define ssize_t SSIZE_T
#endif

int create_udp_server_socket(uint16_t port);
int create_udp_client_socket();
std::string sockaddr_to_ipstr(const sockaddr_in& sa);

#ifdef _WIN32
void initialize_winsock();
void cleanup_winsock();
#endif

ssize_t udp_send(int sock, const void* buf, size_t len, const sockaddr_in* dest_addr);
ssize_t udp_receive(int sock, void* buf, size_t len, sockaddr_in* src_addr);
bool udp_wait_for_socket(int fd, double timeout);
ssize_t udp_receive_packet(int sock, void* packet, size_t packet_size, sockaddr_in* src_addr, double timeout);
