#pragma once

class UDPSocket {
public:
    UDPSocket();
    ~UDPSocket();

    UDPSocket(const UDPSocket&) = delete;
    UDPSocket& operator=(const UDPSocket&) = delete;

    UDPSocket(UDPSocket&& other) noexcept;
    UDPSocket& operator=(UDPSocket&& other) noexcept;

    int get_socket_fd() const;

    void set_reuse_address(bool enable);
    void set_multicast_ttl(int ttl);
    void set_multicast_loopback(bool enable);

    void join_multicast_group(const char* multicast_addr, const char* interface_addr = nullptr);
    void bind(sockaddr_in& addr);

    ssize_t send_to(const char* data, size_t len, const sockaddr_in& dest_addr);
    ssize_t receive_from(char* buffer, size_t len, sockaddr_in& src_addr);

private:
    int m_socket_fd;
};
