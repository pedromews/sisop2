#pragma once
#include <cstdint>
#include "server_state.hpp"

void discovery_server_loop(int sockfd, ServerState& state, uint32_t registration_balance);
