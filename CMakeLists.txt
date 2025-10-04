cmake_minimum_required(VERSION 3.16)
project(UDPClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define include directory
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# === Shared library for common networking code ===
add_library(udp_client_core
    src/MoldUDPReceiver.cpp
    src/UDPSocket.cpp
)

target_include_directories(udp_client_core PUBLIC "${INCLUDE_DIR}")
target_compile_options(udp_client_core PRIVATE -Wall -Wextra -g)

# === simple_client ===
add_executable(simple_client
    src/simple_client.cpp
)
target_link_libraries(simple_client PRIVATE udp_client_core)
target_include_directories(simple_client PRIVATE "${INCLUDE_DIR}")
target_compile_options(simple_client PRIVATE -Wall -Wextra -g)

# === multicast_client ===
add_executable(multicast_client
    src/multicast_client.cpp
)
target_link_libraries(multicast_client PRIVATE udp_client_core)
target_include_directories(multicast_client PRIVATE "${INCLUDE_DIR}")
target_compile_options(multicast_client PRIVATE -Wall -Wextra -g)

# === mold_udp_client ===
add_executable(mold_udp_client
    src/main.cpp
)
target_link_libraries(mold_udp_client PRIVATE udp_client_core)
target_include_directories(mold_udp_client PRIVATE "${INCLUDE_DIR}")
target_compile_options(mold_udp_client PRIVATE -Wall -Wextra -g)
