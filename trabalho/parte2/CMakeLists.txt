cmake_minimum_required(VERSION 3.5)
project(pix_sim)

set(CMAKE_CXX_STANDARD 17)
include_directories(include)

# Server executable
add_executable(server
    src/main_server.cpp
    src/server.cpp
    src/server_state.cpp
    src/udp_util.cpp
    src/discovery.cpp
    src/utils.cpp
    src/replica_manager.cpp
    src/election.cpp
)

# Client executable
add_executable(client
    src/main_client.cpp
    src/client.cpp
    src/udp_util.cpp
    src/utils.cpp
)

# Link pthread library
find_package(Threads REQUIRED)
target_link_libraries(server Threads::Threads)
target_link_libraries(client Threads::Threads)

# Set warning flags
if(MSVC)
    target_compile_options(server PRIVATE /W4)
    target_compile_options(client PRIVATE /W4)
else()
    target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic)
endif()