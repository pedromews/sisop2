#pragma once
#include <cstdint>
#include "server_state.hpp"

using namespace std;

void discovery_server_loop(int sockfd, ServerState& state, uint32_t registration_balance);
