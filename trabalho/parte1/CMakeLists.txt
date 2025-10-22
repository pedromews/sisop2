cmake_minimum_required(VERSION 3.5)
project(pix_sim)

set(CMAKE_CXX_STANDARD 17)
include_directories(include)

add_executable(servidor src/main_server.cpp src/udp_util.cpp src/processing.cpp src/discovery.cpp)
add_executable(cliente src/main_client.cpp src/client.cpp src/udp_util.cpp)
