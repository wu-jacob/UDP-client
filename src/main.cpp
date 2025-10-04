#include <iostream>
#include <string_view>
#include <MoldUDPReceiver.hpp>

constexpr int MULTICAST_PORT = 9000;
constexpr std::string_view MULTICAST_GROUP = "239.1.1.1";

int main() {
    MoldUDPReceiver receiver(MULTICAST_GROUP.data(), MULTICAST_PORT);
    
    std::cout << "\nWaiting for MoldUDP packets... (Ctrl+C to exit)\n";
    
    while (true) {
        receiver.receive_and_process();
    }

    return 0;
}
