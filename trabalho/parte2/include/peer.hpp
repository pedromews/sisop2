#pragma once
#include <netinet/in.h>
#include <cstdint>

struct Peer {
    uint32_t id;
    sockaddr_in addr;
};