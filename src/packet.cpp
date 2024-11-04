/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#include "packet.hpp"

#include <ncurses.h>
#include <ifaddrs.h>
#include "display.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <cstring>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include "connectionsTable.hpp"
#include "connectionID.hpp"
#include "connection.hpp"

// Constructor
PacketCapture::PacketCapture(std::string interfaceName, ConnectionsTable &connectionsTable) : m_connectionsTable(connectionsTable)
{
    m_pcapHandle = nullptr;
    m_interfaceName = interfaceName;
    m_isCapturing = false;
    initLocalAddresses();
    m_dataLinkType = 0;
}
// Destructor
PacketCapture::~PacketCapture()
{
    stopCapture();
}

// Store all IPv4 and IPv6 local addresses for the specified interface
// into m_localIPv6Adresses and m_localIPv4Adresses
void PacketCapture::initLocalAddresses()
{
    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1)
    {
        endwin();
        std::cerr << "Couldn't retrieve interfaces local addresses" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Iterate trough the list of interfaces
    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;
        // Only consider interface that user has specifed
        if (std::string(ifa->ifa_name) != m_interfaceName)
            continue;

        // Get ipv4 addresses
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *sa = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
            in_addr addr_network_order;
            addr_network_order.s_addr = sa->sin_addr.s_addr;
            m_localIPv4Addresses.push_back(addr_network_order);
        }
        // Get ipv6 addresses
        else if (ifa->ifa_addr->sa_family == AF_INET6)
        {
            struct sockaddr_in6 *sa6 = reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr);
            m_localIPv6Addresses.push_back(sa6->sin6_addr);
        }
    }
    // Free the memory
    freeifaddrs(ifaddr);
}

// Check if IPv4 address is local
bool PacketCapture::isLocalIPv4Address(const in_addr &addr)
{
    for (in_addr &localAddr : m_localIPv4Addresses)
    {
        if (localAddr.s_addr == addr.s_addr)
        {
            return true;
        }
    }
    return false;
}

// Check if IPv6 address is local
bool PacketCapture::isLocalIPv6Address(const in6_addr &addr)
{
    for (in6_addr &localAddr : m_localIPv6Addresses)
    {
        if (memcmp(&localAddr, &addr, sizeof(in6_addr)) == 0)
        {
            return true;
        }
    }
    return false;
}

// Starts packet capturing process on the specifed network interface. Set up pcap and enter packets
// processing loop
void PacketCapture::startCapture()
{
    char currentError[PCAP_ERRBUF_SIZE];
    // Open network interface for capture
    m_pcapHandle = pcap_open_live(m_interfaceName.c_str(), BUFSIZ, 1, 1000, currentError);
    if (m_pcapHandle == nullptr)
    {
        endwin();
        std::cerr << "Couldn't open interface " << m_interfaceName << ": " << currentError << std::endl;
        exit(EXIT_FAILURE);
    }
    m_dataLinkType = pcap_datalink(m_pcapHandle);

    // Get data link type to ensure app will correctly work on loopback interface
    switch (m_dataLinkType)
    {
    case DLT_EN10MB:
        m_linkLevelHeaderLen = 14; // Ethernet
        break;
    case DLT_NULL:
        m_linkLevelHeaderLen = 4; // Loopback
        break;
    case DLT_LOOP:
        m_linkLevelHeaderLen = 4; // Loopback
        break;
    default:
        endwin();
        std::cerr << "Unsupported datalink type: " << m_dataLinkType << std::endl;
        exit(EXIT_FAILURE);
    }

    m_isCapturing = true;
    // Main loop, 0 for endless loop
    int loopStatus = pcap_loop(m_pcapHandle, 0, PacketCapture::packetHandler, reinterpret_cast<unsigned char *>(this));
    // Handle loop status
    if (loopStatus == -1)
    {
        endwin();
        std::cerr << "Error during packet capture: " << pcap_geterr(m_pcapHandle) << std::endl;
        exit(EXIT_FAILURE);
    }
    else if (loopStatus == -2)
    {
        std::cerr << "Packet capture was interrupted (pcap_breakloop() called)." << std::endl;
    }
    else
    {
        std::cerr << "Packet capture ended normally." << std::endl;
    }

    pcap_close(m_pcapHandle);
    m_pcapHandle = nullptr;
    m_isCapturing = false;

    std::cerr << "Packet capture stopped." << std::endl;
}

// Method to stop capture and clean up after
void PacketCapture::stopCapture()
{
    if (m_isCapturing && m_pcapHandle != nullptr)
    {
        pcap_breakloop(m_pcapHandle);
        pcap_close(m_pcapHandle);
        m_pcapHandle = nullptr;
        m_isCapturing = false;
    }
}

// Callback function for pcap. Reliable for processing single packet, extract data and update ConnectionsTable
void PacketCapture::packetHandler(unsigned char *packetCaptureObject, const struct pcap_pkthdr *pkthdr, const unsigned char *packet)
{
    uint8_t version = 0;
    uint32_t family = 0;
    PacketCapture *self = reinterpret_cast<PacketCapture *>(packetCaptureObject);
    const unsigned char *ipPacket = packet + self->m_linkLevelHeaderLen;

    // Check family
    if (self->m_dataLinkType == DLT_NULL || self->m_dataLinkType == DLT_LOOP)
    {
        // For DLT_NULL and DLT_LOOP, consider first 4 bytes
        std::memcpy(&family, packet, 4);
    }
    else
    {
        // For other types, family is determine from IP header
        family = 0;
    }

    // Extract IP version
    if (family == AF_INET || (family == 0 && ((ipPacket[0] >> 4) == 4)))
    {
        version = 4;
    }
    else if (family == AF_INET6 || (family == 0 && ((ipPacket[0] >> 4) == 6)))
    {
        version = 6;
    }
    else
    {
        return;
    }

    // Process IPv4 packet
    if (version == 4)
    {
        // Cast the IP header
        const struct ip *ipHeader = reinterpret_cast<const struct ip *>(ipPacket);
        // Extract protocol
        uint8_t protocol = ipHeader->ip_p;
        // Extract IP addresses
        in_addr srcIPv4 = ipHeader->ip_src;
        in_addr destIPv4 = ipHeader->ip_dst;

        // Determine if the packet is outgoing or incoming by comparing address to local addresses
        bool isTransmit = self->isLocalIPv4Address(srcIPv4);
        bool isReceive = self->isLocalIPv4Address(destIPv4);

        switch (protocol)
        {
        // TCP packet
        case IPPROTO_TCP:
        {
            // Cast the TCP header
            const struct tcphdr *tcpHeader = reinterpret_cast<const struct tcphdr *>(ipPacket + (ipHeader->ip_hl * 4));
            // Extract ports
            uint16_t srcPort = ntohs(tcpHeader->th_sport);
            uint16_t destPort = ntohs(tcpHeader->th_dport);
            // Create and construct ConnectionID object
            ConnectionID connID = ConnectionID::storeIPv4InIPv6(srcIPv4, srcPort, destIPv4, destPort, Protocol::TCP);
            // Update ConnectionsTable based on the direction of the packet
            if (isTransmit)
            {
                self->m_connectionsTable.updateConnection(connID, true, pkthdr->len);
            }
            if (isReceive)
            {
                self->m_connectionsTable.updateConnection(connID, false, pkthdr->len);
            }
            break;
        }
        // UDP packet
        case IPPROTO_UDP:
        {
            // Cast the UDP header
            const struct udphdr *udpHeader = reinterpret_cast<const struct udphdr *>(ipPacket + (ipHeader->ip_hl * 4));
            // Extract ports
            uint16_t srcPort = ntohs(udpHeader->uh_sport);
            uint16_t destPort = ntohs(udpHeader->uh_dport);
            // Create and construct ConnectionID object
            ConnectionID connID = ConnectionID::storeIPv4InIPv6(srcIPv4, srcPort, destIPv4, destPort, Protocol::UDP);
            // Update ConnectionsTable based on the direction of the packet
            if (isTransmit)
            {
                self->m_connectionsTable.updateConnection(connID, true, pkthdr->len);
            }
            if (isReceive)
            {
                self->m_connectionsTable.updateConnection(connID, false, pkthdr->len);
            }
            break;
        }
        // ICMP packet
        case IPPROTO_ICMP:
        {
            // ICMP has no ports
            uint16_t srcPort = 0;
            uint16_t destPort = 0;
            // Create and construct ConnectionID object
            ConnectionID connID = ConnectionID::storeIPv4InIPv6(srcIPv4, srcPort, destIPv4, destPort, Protocol::ICMP);
            // Update ConnectionsTable based on the direction of the packet
            if (isTransmit)
            {
                self->m_connectionsTable.updateConnection(connID, true, pkthdr->len);
            }
            if (isReceive)
            {
                self->m_connectionsTable.updateConnection(connID, false, pkthdr->len);
            }
            break;
        }
        }
    }
    // IPv6 packet
    else if (version == 6)
    {
        // Cast the IPv6 header
        const struct ip6_hdr *ip6Header = reinterpret_cast<const struct ip6_hdr *>(ipPacket);
        // Extract protocol
        uint8_t protocol = ip6Header->ip6_nxt;
        // Extract IP addresses
        in6_addr srcIPv6 = ip6Header->ip6_src;
        in6_addr destIPv6 = ip6Header->ip6_dst;
        // Determine the direction of a packet
        bool isTransmit = self->isLocalIPv6Address(srcIPv6);
        bool isReceive = self->isLocalIPv6Address(destIPv6);

        switch (protocol)
        {
        // TCP packet
        case IPPROTO_TCP:
        {
            // Cast the TCP header
            const struct tcphdr *tcpHeader = reinterpret_cast<const struct tcphdr *>(ipPacket + sizeof(struct ip6_hdr));
            // Extract ports
            uint16_t srcPort = ntohs(tcpHeader->th_sport);
            uint16_t destPort = ntohs(tcpHeader->th_dport);

            // Create sockaddr_in6 for source
            sockaddr_in6 srcSockAddr6 = {};
            srcSockAddr6.sin6_family = AF_INET6;
            srcSockAddr6.sin6_addr = srcIPv6;
            srcSockAddr6.sin6_port = htons(srcPort);

            // Create sockaddr_in6 for destination
            sockaddr_in6 destSockAddr6 = {};
            destSockAddr6.sin6_family = AF_INET6;
            destSockAddr6.sin6_addr = destIPv6;
            destSockAddr6.sin6_port = htons(destPort);
            // Initialize and construct ConnectionID object
            ConnectionID connID(srcSockAddr6, destSockAddr6, Protocol::TCP);
            // Update ConnectionsTable depending on the direction of packets
            if (isTransmit)
            {
                self->m_connectionsTable.updateConnection(connID, true, pkthdr->len);
            }
            if (isReceive)
            {
                self->m_connectionsTable.updateConnection(connID, false, pkthdr->len);
            }
            break;
        }
        // UDP packet
        case IPPROTO_UDP:
        {
            // Cast the UDP header
            const struct udphdr *udpHeader = reinterpret_cast<const struct udphdr *>(ipPacket + sizeof(struct ip6_hdr));
            // Extract ports
            uint16_t srcPort = ntohs(udpHeader->uh_sport);
            uint16_t destPort = ntohs(udpHeader->uh_dport);
            // Create sockaddr_in6 for source
            sockaddr_in6 srcSockAddr6 = {};
            srcSockAddr6.sin6_family = AF_INET6;
            srcSockAddr6.sin6_addr = srcIPv6;
            srcSockAddr6.sin6_port = htons(srcPort);
            // Create sockaddr_in6 for destination
            sockaddr_in6 destSockAddr6 = {};
            destSockAddr6.sin6_family = AF_INET6;
            destSockAddr6.sin6_addr = destIPv6;
            destSockAddr6.sin6_port = htons(destPort);
            // Initialize and construct ConnectionID object
            ConnectionID connID(srcSockAddr6, destSockAddr6, Protocol::UDP);
            // Update ConnectionsTable depending on the direction of packets
            if (isTransmit)
            {
                self->m_connectionsTable.updateConnection(connID, true, pkthdr->len);
            }
            if (isReceive)
            {
                self->m_connectionsTable.updateConnection(connID, false, pkthdr->len);
            }
            break;
        }
        // ICMPv6 packet
        case IPPROTO_ICMPV6:
        {
            // ICMPv6 has no ports
            uint16_t srcPort = 0;
            uint16_t destPort = 0;
            // Create sockaddr_in6 for source
            sockaddr_in6 srcSockAddr6 = {};
            srcSockAddr6.sin6_family = AF_INET6;
            srcSockAddr6.sin6_addr = srcIPv6;
            srcSockAddr6.sin6_port = htons(srcPort);
            // Create sockaddr_in6 for destination
            sockaddr_in6 destSockAddr6 = {};
            destSockAddr6.sin6_family = AF_INET6;
            destSockAddr6.sin6_addr = destIPv6;
            destSockAddr6.sin6_port = htons(destPort);
            // Initialize and construct ConnectionID object
            ConnectionID connID(srcSockAddr6, destSockAddr6, Protocol::ICMPv6);
            // Update ConnectionsTable depending on the direction of packets
            if (isTransmit)
            {
                self->m_connectionsTable.updateConnection(connID, true, pkthdr->len);
            }
            if (isReceive)
            {
                self->m_connectionsTable.updateConnection(connID, false, pkthdr->len);
            }
            break;
        }
        }
    }
}