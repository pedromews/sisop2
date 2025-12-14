#pragma once
#include <cstdint>
#include "server_state.hpp"
#include <netinet/in.h>

void handle_discovery_request(int sockfd, ServerState& state, const sockaddr_in& src);
