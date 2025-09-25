cmake_minimum_required(VERSION 3.16)
project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES src/*.cpp)

add_executable(myapp
  ${SOURCES}
)

target_include_directories(myapp PRIVATE include)

target_compile_options(myapp PRIVATE -Wall -Wextra -g)
