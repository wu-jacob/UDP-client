#pragma once

#include <string>
#include <string_view>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <arpa/inet.h>

#include <MoldUDPReceiver.hpp>

class MoldUDPReceiverDPDK {
public:
    MoldUDPReceiverDPDK(const char* multicast_addr, int port, const char* interface_addr = nullptr);
    ~MoldUDPReceiverDPDK();
    
    void receive_and_process();
    const std::string& get_multicast_address() const;
    
    // DPDK-specific initialization
    static void init_dpdk(int argc, char** argv);
    
private:
    void setup_port();
    void configure_multicast();
    void process_packet(rte_mbuf* mbuf);
    void parse_mold_packet(const uint8_t* payload, size_t length, 
                          uint32_t src_ip, uint16_t src_port);
    
    std::string m_multicast_addr;
    uint16_t m_port;
    uint16_t m_dpdk_port_id;
    uint32_t m_multicast_ip;
    
    rte_mempool* m_mbuf_pool;
    
    static constexpr uint16_t RX_RING_SIZE = 1024;
    static constexpr uint16_t TX_RING_SIZE = 1024;
    static constexpr uint16_t NUM_MBUFS = 8191;
    static constexpr uint16_t MBUF_CACHE_SIZE = 250;
    static constexpr uint16_t BURST_SIZE = 32;
};