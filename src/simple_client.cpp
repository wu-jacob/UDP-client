#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;
constexpr std::string_view SERVER_IP = "127.0.0.1";

int main() {
    int socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socket_fd < 0) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    sockaddr_in server_addr {};

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    // inet_pton converts IPv4 and IPv6 addresses from text to binary format
    // If the address is invalid, it returns 0. On error, it returns -1.
    if (inet_pton(AF_INET, SERVER_IP.data(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        close(socket_fd);
        return -1;
    }

    std::cout << "UDP Client started. Type messages (or 'quit' to exit):\n";

    std::string message;

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, message)) {
            /*
            Handle EOF (Ctrl+D)
            When getline receives EOF, it sets the failbit. This causes getline to enter a fail state.
            In this fail state, subsequent calls to getline will fail until the state is cleared.
            This means that we will be stuck in an infinite loop of failed getline calls.
            */
            break;
        }

        if (message == "quit") {
            break;
        }

        ssize_t sent_bytes = sendto(socket_fd, message.data(), message.size(), 0,
            reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

        if (sent_bytes < 0) {
            std::cerr << "Error sending data\n";
            continue;
        }

        char buffer[BUFFER_SIZE] = {};
        socklen_t addr_len = sizeof(server_addr);

        /*
        We pass the server_addr to recvfrom to get the address of the sender
        Note that compared to the sendto function, the addr_len parameter is a pointer for this reason
        */
        ssize_t recv_bytes = recvfrom(socket_fd, buffer, BUFFER_SIZE - 1,
            0, reinterpret_cast<sockaddr*>(&server_addr), &addr_len);

        if (recv_bytes < 0) {
            std::cerr << "Error receiving data\n";
            continue;
        }

        buffer[recv_bytes] = '\0'; // Null-terminate the received data
        std::cout << "Server: " << buffer << "\n";
    }

    close(socket_fd);
    return 0;
};
