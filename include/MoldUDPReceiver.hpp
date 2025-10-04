#pragma once

#include <netinet/in.h>

#include <UDPSocket.hpp>

struct MoldUDP64PacketHeader {
    // All multi-byte integer fields are in network byte order (big-endian)
    char m_session[10]; // Alphanumeric session to which the packet belongs
    uint64_t m_sequence_number; // Sequence number of the first message in the packet
    uint16_t m_message_count; // The count of messages in the packet

    std::string get_session() const;
    void set_session(std::string_view session);

    uint64_t get_sequence_number() const;
    void set_sequence_number(uint64_t seq_num);

    uint16_t get_message_count() const;
    void set_message_count(uint16_t msg_count);

} __attribute__((packed));

struct MoldUDP64MessageHeader {
    uint16_t message_length; // Length in bytes of the message block, excluding this header

    uint16_t get_message_length() const;
    void set_message_length(uint16_t len);
} __attribute__((packed));

class MoldUDPReceiver {
public:
    MoldUDPReceiver(const char* multicast_addr, int port, const char* interface_addr = nullptr);

    void receive_and_process();

    const std::string& get_multicast_address() const;

private:
    UDPSocket m_socket;
    std::string m_multicast_addr {};
    sockaddr_in m_local_addr {};
};