#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#include "UDPSocket.hpp"

UDPSocket::UDPSocket()
    : m_socket_fd(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) {

    if (m_socket_fd < 0) {
        throw std::runtime_error("Failed to create UDP socket");
    }
}

UDPSocket::~UDPSocket() {
    if (m_socket_fd >= 0) {
        close(m_socket_fd);
    }
}

UDPSocket::UDPSocket(UDPSocket&& other) noexcept
    : m_socket_fd(other.m_socket_fd) {
    other.m_socket_fd = -1;
}

UDPSocket& UDPSocket::operator=(UDPSocket&& other) noexcept {
    if (this != &other) {
        if (m_socket_fd >= 0) {
            close(m_socket_fd);
        }
        m_socket_fd = other.m_socket_fd;
        other.m_socket_fd = -1;
    }

    return *this;
}

void UDPSocket::set_reuse_address(bool enable) {
    int reuse = enable ? 1 : 0;
    
    if (setsockopt(m_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }
}

void UDPSocket::set_multicast_ttl(int ttl) {
    if (setsockopt(m_socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        throw std::runtime_error("Failed to set IP_MULTICAST_TTL");
    }
}

void UDPSocket::set_multicast_loopback(bool enable) {
    int loopback = enable ? 1 : 0;

    if (setsockopt(m_socket_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback)) < 0) {
        throw std::runtime_error("Failed to set IP_MULTICAST_LOOP");
    }
}

void UDPSocket::join_multicast_group(const char* multicast_addr, const char* interface_addr) {
    ip_mreq mreq {};
    
    if (inet_pton(AF_INET, multicast_addr, &mreq.imr_multiaddr) <= 0) {
        throw std::runtime_error("Invalid multicast address");
    }

    if (interface_addr) {
        if (inet_pton(AF_INET, interface_addr, &mreq.imr_interface) <= 0) {
            throw std::runtime_error("Invalid interface address");
        }
    } else {
        mreq.imr_interface.s_addr = INADDR_ANY;
    }

    if (setsockopt(m_socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        throw std::runtime_error("Failed to join multicast group");
    }
}

void UDPSocket::bind(sockaddr_in& addr) {
    if (::bind(m_socket_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind UDP socket");
    }
}

ssize_t UDPSocket::send_to(const char* data, size_t len, const sockaddr_in& dest_addr) {
    ssize_t bytes_sent = sendto(m_socket_fd, data, len, 0,
        reinterpret_cast<const sockaddr*>(&dest_addr), sizeof(dest_addr));

    if (bytes_sent < 0) {
        throw std::runtime_error("Failed to send data");
    }

    return bytes_sent;
}

ssize_t UDPSocket::receive_from(char* buffer, size_t len, sockaddr_in& src_addr) {
    socklen_t addr_len = sizeof(src_addr);

    ssize_t bytes_received = recvfrom(m_socket_fd, buffer, len, 0,
        reinterpret_cast<sockaddr*>(&src_addr), &addr_len);

    if (bytes_received < 0) {
        throw std::runtime_error("Failed to receive data");
    }

    return bytes_received;
}
