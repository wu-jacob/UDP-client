#include <iostream>
#include <string_view>
#include <csignal>
#include <MoldUDPReceiverDPDK.hpp>

constexpr int MULTICAST_PORT = 9000;
constexpr std::string_view MULTICAST_GROUP = "239.1.1.1";

volatile bool keep_running = true;

void signal_handler(int signum) {
    std::cout << "\nShutting down...\n";
    keep_running = false;
}

int main(int argc, char** argv) {
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        /*
        Initialize DPDK EAL (Environment Abstraction Layer)
        Common DPDK arguments:
        -l: logical cores to use (e.g., -l 0-1)
        -n: memory channels (e.g., -n 4)
        --: separator between EAL and application arguments
        */
        
        std::cout << "Initializing DPDK...\n";
        MoldUDPReceiverDPDK::init_dpdk(argc, argv);
        
        MoldUDPReceiverDPDK receiver(MULTICAST_GROUP.data(), MULTICAST_PORT);
        
        std::cout << "\nWaiting for MoldUDP packets... (Ctrl+C to exit)\n";
        std::cout << "Zero-copy processing enabled via DPDK\n\n";
        
        while (keep_running) {
            receiver.receive_and_process();
        }
        
        std::cout << "Receiver stopped gracefully\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
