cmake_minimum_required(VERSION 3.16)
project(UDPClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define include directory
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# Common compile options
set(COMMON_COMPILE_OPTIONS -Wall -Wextra -g)

# ============================================================================
# SOCKET-BASED IMPLEMENTATIONS (Original)
# ============================================================================

# === Shared library for common networking code ===
add_library(udp_client_core
    src/MoldUDPReceiver.cpp
    src/UDPSocket.cpp
)

target_include_directories(udp_client_core PUBLIC "${INCLUDE_DIR}")
target_compile_options(udp_client_core PRIVATE ${COMMON_COMPILE_OPTIONS})

# === simple_client ===
add_executable(simple_client
    src/simple_client.cpp
)
target_link_libraries(simple_client PRIVATE udp_client_core)
target_include_directories(simple_client PRIVATE "${INCLUDE_DIR}")
target_compile_options(simple_client PRIVATE ${COMMON_COMPILE_OPTIONS})

# === multicast_client ===
add_executable(multicast_client
    src/multicast_client.cpp
)
target_link_libraries(multicast_client PRIVATE udp_client_core)
target_include_directories(multicast_client PRIVATE "${INCLUDE_DIR}")
target_compile_options(multicast_client PRIVATE ${COMMON_COMPILE_OPTIONS})

# === mold_udp_client ===
add_executable(mold_udp_client
    src/main.cpp
)
target_link_libraries(mold_udp_client PRIVATE udp_client_core)
target_include_directories(mold_udp_client PRIVATE "${INCLUDE_DIR}")
target_compile_options(mold_udp_client PRIVATE ${COMMON_COMPILE_OPTIONS})

# ============================================================================
# DPDK-BASED IMPLEMENTATION (High Performance)
# ============================================================================

# Find pkg-config
find_package(PkgConfig)

if(PkgConfig_FOUND)
    # Try to find DPDK using pkg-config
    pkg_check_modules(DPDK libdpdk)
    
    if(DPDK_FOUND)
        message(STATUS "DPDK found: ${DPDK_VERSION}")
        message(STATUS "DPDK include dirs: ${DPDK_INCLUDE_DIRS}")
        message(STATUS "DPDK library dirs: ${DPDK_LIBRARY_DIRS}")
        message(STATUS "DPDK libraries: ${DPDK_LIBRARIES}")
        
        # === DPDK MoldUDP Receiver Library ===
        add_library(mold_udp_dpdk_core
            src/MoldUDPReceiverDPDK.cpp
            src/MoldUDPReceiver.cpp
            src/UDPSocket.cpp
        )
        
        target_include_directories(mold_udp_dpdk_core PUBLIC
            "${INCLUDE_DIR}"
            ${DPDK_INCLUDE_DIRS}
        )
        
        target_compile_options(mold_udp_dpdk_core PRIVATE
            ${COMMON_COMPILE_OPTIONS}
            ${DPDK_CFLAGS_OTHER}
        )
        
        target_link_directories(mold_udp_dpdk_core PUBLIC
            ${DPDK_LIBRARY_DIRS}
        )
        
        target_link_libraries(mold_udp_dpdk_core PUBLIC
            ${DPDK_LIBRARIES}
            pthread
            dl
            numa  # Optional: for NUMA awareness
        )
        
        # === DPDK MoldUDP Client Executable ===
        add_executable(mold_udp_dpdk_client
            src/main_dpdk.cpp
        )
        
        target_include_directories(mold_udp_dpdk_client PRIVATE
            "${INCLUDE_DIR}"
            ${DPDK_INCLUDE_DIRS}
        )
        
        target_compile_options(mold_udp_dpdk_client PRIVATE
            ${COMMON_COMPILE_OPTIONS}
            ${DPDK_CFLAGS_OTHER}
        )
        
        target_link_libraries(mold_udp_dpdk_client PRIVATE
            mold_udp_dpdk_core
        )
        
        # Set RPATH for runtime library discovery
        set_target_properties(mold_udp_dpdk_client PROPERTIES
            INSTALL_RPATH "${DPDK_LIBRARY_DIRS}"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
        
        message(STATUS "DPDK targets enabled: mold_udp_dpdk_client")
        
    else()
        message(WARNING "DPDK not found via pkg-config. DPDK targets will not be built.")
        message(STATUS "To install DPDK:")
        message(STATUS "  Ubuntu/Debian: sudo apt-get install dpdk dpdk-dev")
        message(STATUS "  Or build from source: https://doc.dpdk.org/guides/linux_gsg/build_dpdk.html")
    endif()
else()
    message(WARNING "pkg-config not found. DPDK targets will not be built.")
endif()

# ============================================================================
# Installation Rules
# ============================================================================

install(TARGETS 
    simple_client 
    multicast_client 
    mold_udp_client
    RUNTIME DESTINATION bin
)

if(DPDK_FOUND)
    install(TARGETS mold_udp_dpdk_client
        RUNTIME DESTINATION bin
    )
endif()

# ============================================================================
# Print Build Summary
# ============================================================================

message(STATUS "")
message(STATUS "=== Build Configuration Summary ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
message(STATUS "Socket-based targets:")
message(STATUS "  - simple_client")
message(STATUS "  - multicast_client")
message(STATUS "  - mold_udp_client")
message(STATUS "")
if(DPDK_FOUND)
    message(STATUS "DPDK-based targets: ENABLED")
    message(STATUS "  - mold_udp_dpdk_client")
    message(STATUS "  DPDK Version: ${DPDK_VERSION}")
else()
    message(STATUS "DPDK-based targets: DISABLED (DPDK not found)")
endif()
message(STATUS "===================================")
message(STATUS "")
