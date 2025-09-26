#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

/*
Allow multiple clients on the same machine to bind to the same port
This is necessary for testing multicast locally
*/
constexpr int REUSE = 1;

constexpr int BUFFER_SIZE = 1024;
constexpr int MULTICAST_PORT = 9000;
constexpr std::string_view MULTICAST_GROUP = "239.1.1.1";

int main() {
    int socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socket_fd < 0) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &REUSE, sizeof(REUSE)) < 0) {
        std::cerr << "Setting SO_REUSEADDR failed\n";
        close(socket_fd);
        return -1;
    }

    sockaddr_in local_addr {};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(MULTICAST_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr)) < 0) {
        std::cerr << "Binding socket failed\n";
        close(socket_fd);
        return -1;
    }

    ip_mreq multicast_request {};

    if (inet_pton(AF_INET, MULTICAST_GROUP.data(), &multicast_request.imr_multiaddr) <= 0) {
        std::cerr << "Invalid multicast address\n";
        close(socket_fd);
        return -1;
    }

    multicast_request.imr_interface.s_addr = INADDR_ANY;

    // Makes a system call to join the multicast group
    if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast_request, sizeof(multicast_request)) < 0) {
        std::cerr << "Joining multicast group failed\n";
        close(socket_fd);
        return -1;
    }

    std::cout << "Multicast Client started. Listening for messages on " << MULTICAST_GROUP << ":" << MULTICAST_PORT << "\n";

    while (true) {
        char buffer[BUFFER_SIZE];
        ssize_t recv_bytes = recvfrom(socket_fd, buffer, BUFFER_SIZE - 1, 0, nullptr, nullptr);

        if (recv_bytes < 0) {
            std::cerr << "Error receiving data\n";
            continue;
        }

        buffer[recv_bytes] = '\0'; // Null-terminate the received data
        
        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &multicast_request.imr_multiaddr, sender_ip, INET_ADDRSTRLEN); // Convert sender address to string
        std::cout << "Received from " << sender_ip << ": " << buffer << "\n";
    }

    // Make a system call to leave the multicast group
    setsockopt(socket_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &multicast_request, sizeof(multicast_request));
    close(socket_fd);

    return 0;
}
