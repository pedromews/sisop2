#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

int create_udp_server_socket(uint16_t port);
int create_udp_client_socket();
std::string sockaddr_to_ipstr(const sockaddr_in& sa);

ssize_t udp_send(int sock, const void* buf, size_t len, const sockaddr_in* dest_addr);
ssize_t udp_receive(int sock, void* buf, size_t len, sockaddr_in* src_addr);
bool udp_wait_for_socket(int fd, double timeout);
ssize_t udp_receive_packet(int sock, void* packet, size_t packet_size, sockaddr_in* src_addr, double timeout);
