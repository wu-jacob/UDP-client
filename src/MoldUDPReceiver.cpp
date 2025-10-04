#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <iostream>
#include <MoldUDPReceiver.hpp>
#include <stdexcept>
#include <UDPSocket.hpp>

constexpr int MAX_PACKET_SIZE = 1400;

std::string MoldUDP64PacketHeader::get_session() const {
    return std::string(m_session, 10);
}

void MoldUDP64PacketHeader::set_session(std::string_view session) {
    std::fill(std::begin(m_session), std::end(m_session), ' '); // Any unused bytes are space-padded as per spec
    std::copy_n(session.data(), std::min(session.size(), size_t(10)), m_session);
}

uint64_t MoldUDP64PacketHeader::get_sequence_number() const {
    return be64toh(m_sequence_number);
}

void MoldUDP64PacketHeader::set_sequence_number(uint64_t seq_num) {
    m_sequence_number = htobe64(seq_num);
}

uint16_t MoldUDP64PacketHeader::get_message_count() const {
    return ntohs(m_message_count);
}


void MoldUDP64PacketHeader::set_message_count(uint16_t msg_count) {
    m_message_count = htons(msg_count);
}

uint16_t MoldUDP64MessageHeader::get_message_length() const {
    return ntohs(message_length);
}

void MoldUDP64MessageHeader::set_message_length(uint16_t len) {
    message_length = htons(len);
}

MoldUDPReceiver::MoldUDPReceiver(const char* multicast_addr, int port, const char* interface_addr) 
    : m_socket()
    , m_multicast_addr(multicast_addr) {

    m_socket.set_reuse_address(true);

    m_local_addr = {};
    m_local_addr.sin_family = AF_INET;
    m_local_addr.sin_port = htons(port);
    m_local_addr.sin_addr.s_addr = INADDR_ANY;

    m_socket.bind(m_local_addr);

    m_socket.join_multicast_group(multicast_addr, interface_addr);
        
    std::cout << "MoldUDP Multicast Receiver started\n";
    std::cout << "Listening to multicast group: " << multicast_addr 
                << ":" << port << "\n";
    if (interface_addr) {
        std::cout << "On interface: " << interface_addr << "\n";
    } else {
        std::cout << "On all interfaces\n";
    }
}

void MoldUDPReceiver::receive_and_process() {
    std::array<char, MAX_PACKET_SIZE> buffer {};
    sockaddr_in sender_addr {};
    
    ssize_t received = m_socket.receive_from(buffer.data(), buffer.size(), sender_addr);
    
    // Convert sender IP to string for display
    char sender_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, INET_ADDRSTRLEN);
    
    std::cout << "\n=== Received UDP packet from " << sender_ip 
                << ":" << ntohs(sender_addr.sin_port) 
                << " (" << received << " bytes) ===\n";
    
    if (received < static_cast<ssize_t>(sizeof(MoldUDP64PacketHeader))) {
        std::cerr << "Packet too small for MoldUDP header\n";
        return;
    }
    
    const MoldUDP64PacketHeader* session_header = 
        reinterpret_cast<const MoldUDP64PacketHeader*>(buffer.data());
    
    std::cout << "Session: '" << session_header->get_session() << "'\n";
    std::cout << "Sequence: " << session_header->get_sequence_number() << "\n";
    std::cout << "Message Count: " << session_header->get_message_count() << "\n";
    
    size_t offset = sizeof(MoldUDP64PacketHeader);
    uint16_t msg_count = session_header->get_message_count();
    
    for (int i = 0; i < msg_count; i++) {
        if (offset + sizeof(MoldUDP64MessageHeader) > static_cast<size_t>(received)) {
            std::cerr << "Incomplete message header\n";
            break;
        }
        
        const MoldUDP64MessageHeader* msg_header = 
            reinterpret_cast<const MoldUDP64MessageHeader*>(buffer.data() + offset);
        offset += sizeof(MoldUDP64MessageHeader);
        
        uint16_t msg_len = msg_header->get_message_length();
        
        if (offset + msg_len > static_cast<size_t>(received)) {
            std::cerr << "Incomplete message data\n";
            break;
        }
        
        std::string_view message(buffer.data() + offset, msg_len);
        std::cout << "  Message " << (i + 1) << " [" << msg_len << " bytes]: " << message << "\n";
        
        offset += msg_len;
    }
}

const std::string& MoldUDPReceiver::get_multicast_address() const {
    return m_multicast_addr;
}