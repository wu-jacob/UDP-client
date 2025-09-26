cmake_minimum_required(VERSION 3.16)
project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES src/*.cpp)

add_executable(simple_client
  src/simple_client.cpp
)

add_executable(multicast_client
  src/multicast_client.cpp
)

target_include_directories(simple_client PRIVATE include)
target_include_directories(multicast_client PRIVATE include)

target_compile_options(simple_client PRIVATE -Wall -Wextra -g)
target_compile_options(multicast_client PRIVATE -Wall -Wextra -g)
