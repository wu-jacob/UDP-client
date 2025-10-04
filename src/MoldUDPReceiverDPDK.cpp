#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <MoldUDPReceiverDPDK.hpp>
#include <rte_ether.h>

void MoldUDPReceiverDPDK::init_dpdk(int argc, char** argv) {
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        throw std::runtime_error("Failed to initialize DPDK EAL");
    }
    
    uint16_t nb_ports = rte_eth_dev_count_avail();
    std::cout << "DPDK initialized. Available ports: " << nb_ports << "\n";
    
    if (nb_ports == 0) {
        throw std::runtime_error("No DPDK-compatible network ports found");
    }
}

MoldUDPReceiverDPDK::MoldUDPReceiverDPDK(const char* multicast_addr, int port, const char* interface_addr)
    : m_multicast_addr(multicast_addr)
    , m_port(port)
    , m_dpdk_port_id(0)  // Use first available port
    , m_mbuf_pool(nullptr) {
    
    if (inet_pton(AF_INET, multicast_addr, &m_multicast_ip) != 1) {
        throw std::runtime_error("Invalid multicast address");
    }

    m_multicast_ip = ntohl(m_multicast_ip);
    
    // Create memory pool for packet buffers (zero-copy)
    m_mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL",
        NUM_MBUFS,
        MBUF_CACHE_SIZE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id()
    );
    
    if (m_mbuf_pool == nullptr) {
        throw std::runtime_error("Failed to create mbuf pool");
    }
    
    setup_port();
    configure_multicast();
    
    std::cout << "DPDK MoldUDP Multicast Receiver started\n";
    std::cout << "Listening to multicast group: " << multicast_addr 
              << ":" << port << "\n";
    std::cout << "Using DPDK port: " << m_dpdk_port_id << "\n";
}

MoldUDPReceiverDPDK::~MoldUDPReceiverDPDK() {
    if (m_dpdk_port_id < RTE_MAX_ETHPORTS) {
        rte_eth_dev_stop(m_dpdk_port_id);
        rte_eth_dev_close(m_dpdk_port_id);
    }
}

void MoldUDPReceiverDPDK::setup_port() {
    rte_eth_conf port_conf = {};
    
    // Enable multicast reception
    port_conf.rxmode.mq_mode = RTE_ETH_MQ_RX_NONE;
    port_conf.rxmode.offloads = RTE_ETH_RX_OFFLOAD_CHECKSUM;
    
    rte_eth_dev_info dev_info;
    int ret = rte_eth_dev_info_get(m_dpdk_port_id, &dev_info);
    
    if (ret != 0) {
        throw std::runtime_error("Failed to get device info");
    }
    
    // Configure the Ethernet device
    ret = rte_eth_dev_configure(m_dpdk_port_id, 1, 1, &port_conf);

    if (ret < 0) {
        throw std::runtime_error("Failed to configure port");
    }
    
    // Setup RX queue
    ret = rte_eth_rx_queue_setup(
        m_dpdk_port_id,
        0,  // Queue ID
        RX_RING_SIZE,
        rte_eth_dev_socket_id(m_dpdk_port_id),
        nullptr,
        m_mbuf_pool
    );
    if (ret < 0) {
        throw std::runtime_error("Failed to setup RX queue");
    }
    
    // Setup TX queue (needed even if we don't transmit)
    ret = rte_eth_tx_queue_setup(
        m_dpdk_port_id,
        0,
        TX_RING_SIZE,
        rte_eth_dev_socket_id(m_dpdk_port_id),
        nullptr
    );
    if (ret < 0) {
        throw std::runtime_error("Failed to setup TX queue");
    }
    
    ret = rte_eth_dev_start(m_dpdk_port_id);

    if (ret < 0) {
        throw std::runtime_error("Failed to start port");
    }
    
    // Enable promiscuous mode to receive multicast
    ret = rte_eth_promiscuous_enable(m_dpdk_port_id);
    
    if (ret != 0) {
        std::cerr << "Warning: Failed to enable promiscuous mode\n";
    }
}

void MoldUDPReceiverDPDK::configure_multicast() {
    // Note: Actual multicast group joining depends on NIC capabilities
    // Some NICs require using flow rules or filters
    // This is a simplified version - production code might need flow API
    
    std::cout << "Multicast filtering configured for " << m_multicast_addr << "\n";
}

void MoldUDPReceiverDPDK::receive_and_process() {
    rte_mbuf* bufs[BURST_SIZE];
    
    const uint16_t nb_rx = rte_eth_rx_burst(m_dpdk_port_id, 0, bufs, BURST_SIZE);
    
    for (uint16_t i = 0; i < nb_rx; i++) {
        process_packet(bufs[i]);
        rte_pktmbuf_free(bufs[i]);
    }
}

void MoldUDPReceiverDPDK::process_packet(rte_mbuf* mbuf) {
    uint8_t* packet_data = rte_pktmbuf_mtod(mbuf, uint8_t*);
    size_t packet_len = rte_pktmbuf_pkt_len(mbuf);
    
    rte_ether_hdr* eth_hdr = reinterpret_cast<rte_ether_hdr*>(packet_data);
    
    if (rte_be_to_cpu_16(eth_hdr->ether_type) != RTE_ETHER_TYPE_IPV4) {
        return;
    }
    
    rte_ipv4_hdr* ip_hdr = reinterpret_cast<rte_ipv4_hdr*>(
        packet_data + sizeof(rte_ether_hdr)
    );
    
    if (ip_hdr->next_proto_id != IPPROTO_UDP) {
        return;
    }
    
    uint32_t dst_ip = rte_be_to_cpu_32(ip_hdr->dst_addr);

    if (dst_ip != m_multicast_ip) {
        return;
    }
    
    size_t ip_hdr_len = (ip_hdr->version_ihl & 0x0F) * 4;
    rte_udp_hdr* udp_hdr = reinterpret_cast<rte_udp_hdr*>(
        reinterpret_cast<uint8_t*>(ip_hdr) + ip_hdr_len
    );
    
    uint16_t dst_port = rte_be_to_cpu_16(udp_hdr->dst_port);

    if (dst_port != m_port) {
        return;
    }
    
    // Extract UDP payload (zero-copy pointer to MoldUDP data)
    uint8_t* payload = reinterpret_cast<uint8_t*>(udp_hdr) + sizeof(rte_udp_hdr);
    size_t payload_len = rte_be_to_cpu_16(udp_hdr->dgram_len) - sizeof(rte_udp_hdr);
    
    uint32_t src_ip = rte_be_to_cpu_32(ip_hdr->src_addr);
    uint16_t src_port = rte_be_to_cpu_16(udp_hdr->src_port);
    
    parse_mold_packet(payload, payload_len, src_ip, src_port);
}

void MoldUDPReceiverDPDK::parse_mold_packet(const uint8_t* payload, size_t length,
    uint32_t src_ip, uint16_t src_port) {
    char sender_ip[INET_ADDRSTRLEN];
    struct in_addr addr;
    addr.s_addr = htonl(src_ip);
    inet_ntop(AF_INET, &addr, sender_ip, INET_ADDRSTRLEN);
    
    std::cout << "\n=== Received UDP packet from " << sender_ip 
              << ":" << src_port 
              << " (" << length << " bytes) ===\n";
    
    if (length < sizeof(MoldUDP64PacketHeader)) {
        std::cerr << "Packet too small for MoldUDP header\n";
        return;
    }
    
    // Zero-copy: cast payload directly to MoldUDP header
    const MoldUDP64PacketHeader* session_header = 
        reinterpret_cast<const MoldUDP64PacketHeader*>(payload);
    
    std::cout << "Session: '" << session_header->get_session() << "'\n";
    std::cout << "Sequence: " << session_header->get_sequence_number() << "\n";
    std::cout << "Message Count: " << session_header->get_message_count() << "\n";
    
    size_t offset = sizeof(MoldUDP64PacketHeader);
    uint16_t msg_count = session_header->get_message_count();
    
    for (int i = 0; i < msg_count; i++) {
        if (offset + sizeof(MoldUDP64MessageHeader) > length) {
            std::cerr << "Incomplete message header\n";
            break;
        }
        
        const MoldUDP64MessageHeader* msg_header = 
            reinterpret_cast<const MoldUDP64MessageHeader*>(payload + offset);
        offset += sizeof(MoldUDP64MessageHeader);
        
        uint16_t msg_len = msg_header->get_message_length();
        
        if (offset + msg_len > length) {
            std::cerr << "Incomplete message data\n";
            break;
        }
        
        // Zero-copy: create string_view directly from packet buffer
        std::string_view message(
            reinterpret_cast<const char*>(payload + offset), 
            msg_len
        );
        std::cout << "  Message " << (i + 1) << " [" << msg_len 
            << " bytes]: " << message << "\n";
        
        offset += msg_len;
    }
}

const std::string& MoldUDPReceiverDPDK::get_multicast_address() const {
    return m_multicast_addr;
}