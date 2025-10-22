#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

int create_udp_server_socket(uint16_t port);
int create_udp_client_socket();
string sockaddr_to_ipstr(const sockaddr_in& sa);
