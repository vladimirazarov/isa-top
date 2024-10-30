#include "packet.hpp"

#include <ifaddrs.h>
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

PacketCapture::PacketCapture(std::string interfaceName, ConnectionsTable &connectionsTable) : m_connectionsTable(connectionsTable)
{
    m_pcapHandle = nullptr;
    m_interfaceName = interfaceName;
    m_isCapturing = false;
    initLocalAddresses();
}
PacketCapture::~PacketCapture()
{
    stopCapture();
}

void PacketCapture::initLocalAddresses()
{
    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1)
    {
        std::cerr << "Couldn't retrieve interfaces local addresses" << std::endl;
        exit(EXIT_FAILURE);
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;

        if (std::string(ifa->ifa_name) != m_interfaceName)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            in_addr addr_network_order;
            addr_network_order.s_addr = sa->sin_addr.s_addr;
            m_localIPv4Addresses.push_back(addr_network_order);
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6)
        {
            struct sockaddr_in6 *sa6 = reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr);
            m_localIPv6Addresses.push_back(sa6->sin6_addr);
        }
    }
    freeifaddrs(ifaddr);
}

bool PacketCapture::isLocalIPv4Address(const in_addr& addr) {
    for (in_addr& localAddr : m_localIPv4Addresses) {
        if (localAddr.s_addr == addr.s_addr) {
            return true;
        }
    }
    return false;
}
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
void PacketCapture::startCapture()
{
    char currentError[PCAP_ERRBUF_SIZE];

    m_pcapHandle = pcap_open_live(m_interfaceName.c_str(), BUFSIZ, 1, 1000, currentError);
    if (m_pcapHandle == nullptr)
    {
        std::cerr << "Couldn't open interface " << m_interfaceName << ": " << currentError << std::endl;
        exit(EXIT_FAILURE);
    }
    uint datalinkType = pcap_datalink(m_pcapHandle);

    switch (datalinkType)
    {
    case DLT_EN10MB:
        m_linkLevelHeaderLen = 14;
        break;
    case DLT_NULL:
        m_linkLevelHeaderLen = 4;
        break;
    case DLT_LOOP:
        m_linkLevelHeaderLen = 4;
        break;
    default:
        std::cerr << "Unsupported datalink type: " << datalinkType << std::endl;
        exit(EXIT_FAILURE);
    }

    m_isCapturing = true;
    int loopStatus = pcap_loop(m_pcapHandle, 0, PacketCapture::packetHandler, reinterpret_cast<unsigned char *>(this));

    if (loopStatus == -1)
    {
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
void PacketCapture::packetHandler(unsigned char *packetCaptureObject, const struct pcap_pkthdr *pkthdr, const unsigned char *packet)
{
    uint8_t version = 0;
    uint32_t family = 0;
    PacketCapture *self = reinterpret_cast<PacketCapture *>(packetCaptureObject);
    const unsigned char *ipPacket = packet + self->m_linkLevelHeaderLen;

    if (self->m_dataLinkType == DLT_NULL || self->m_dataLinkType == DLT_LOOP)
    {
        std::memcpy(&family, packet, 4);
    }
    else
    {
        family = 0;
    }

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

    if (version == 4)
    {
        const struct ip *ipHeader = reinterpret_cast<const struct ip *>(ipPacket);
        uint8_t protocol = ipHeader->ip_p;

        in_addr srcIPv4 = ipHeader->ip_src;
        in_addr destIPv4 = ipHeader->ip_dst;

        bool isTransmit = self->isLocalIPv4Address(srcIPv4);
        bool isReceive = self->isLocalIPv4Address(destIPv4);

        switch (protocol)
        {
        case IPPROTO_TCP:
        {
            const struct tcphdr *tcpHeader = reinterpret_cast<const struct tcphdr *>(ipPacket + (ipHeader->ip_hl * 4));
            uint16_t srcPort = ntohs(tcpHeader->th_sport);
            uint16_t destPort = ntohs(tcpHeader->th_dport);
            ConnectionID connID = ConnectionID::storeIPv4InIPv6(srcIPv4, srcPort, destIPv4, destPort, Protocol::TCP);
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

        case IPPROTO_UDP:
        {
            const struct udphdr *udpHeader = reinterpret_cast<const struct udphdr *>(ipPacket + (ipHeader->ip_hl * 4));
            uint16_t srcPort = ntohs(udpHeader->uh_sport);
            uint16_t destPort = ntohs(udpHeader->uh_dport);
            ConnectionID connID = ConnectionID::storeIPv4InIPv6(srcIPv4, srcPort, destIPv4, destPort, Protocol::UDP);
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

        case IPPROTO_ICMP:
        {
            uint16_t srcPort = 0;
            uint16_t destPort = 0;
            ConnectionID connID = ConnectionID::storeIPv4InIPv6(srcIPv4, srcPort, destIPv4, destPort, Protocol::ICMP);
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
    else if (version == 6)
    {
        const struct ip6_hdr *ip6Header = reinterpret_cast<const struct ip6_hdr *>(ipPacket);
        uint8_t protocol = ip6Header->ip6_nxt;

        in6_addr srcIPv6 = ip6Header->ip6_src;
        in6_addr destIPv6 = ip6Header->ip6_dst;

        bool isTransmit = self->isLocalIPv6Address(srcIPv6);
        bool isReceive = self->isLocalIPv6Address(destIPv6);

        switch (protocol)
        {
        case IPPROTO_TCP:
        {
            const struct tcphdr *tcpHeader = reinterpret_cast<const struct tcphdr *>(ipPacket + sizeof(struct ip6_hdr));
            uint16_t srcPort = ntohs(tcpHeader->th_sport);
            uint16_t destPort = ntohs(tcpHeader->th_dport);

            sockaddr_in6 srcSockAddr6 = {};
            srcSockAddr6.sin6_family = AF_INET6;
            srcSockAddr6.sin6_addr = srcIPv6;
            srcSockAddr6.sin6_port = htons(srcPort);

            sockaddr_in6 destSockAddr6 = {};
            destSockAddr6.sin6_family = AF_INET6;
            destSockAddr6.sin6_addr = destIPv6;
            destSockAddr6.sin6_port = htons(destPort);

            ConnectionID connID(srcSockAddr6, destSockAddr6, Protocol::TCP);
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

        case IPPROTO_UDP:
        {
            const struct udphdr *udpHeader = reinterpret_cast<const struct udphdr *>(ipPacket + sizeof(struct ip6_hdr));
            uint16_t srcPort = ntohs(udpHeader->uh_sport);
            uint16_t destPort = ntohs(udpHeader->uh_dport);

            sockaddr_in6 srcSockAddr6 = {};
            srcSockAddr6.sin6_family = AF_INET6;
            srcSockAddr6.sin6_addr = srcIPv6;
            srcSockAddr6.sin6_port = htons(srcPort);

            sockaddr_in6 destSockAddr6 = {};
            destSockAddr6.sin6_family = AF_INET6;
            destSockAddr6.sin6_addr = destIPv6;
            destSockAddr6.sin6_port = htons(destPort);

            ConnectionID connID(srcSockAddr6, destSockAddr6, Protocol::UDP);
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

        case IPPROTO_ICMPV6:
        {
            uint16_t srcPort = 0;
            uint16_t destPort = 0;

            sockaddr_in6 srcSockAddr6 = {};
            srcSockAddr6.sin6_family = AF_INET6;
            srcSockAddr6.sin6_addr = srcIPv6;
            srcSockAddr6.sin6_port = htons(srcPort);

            sockaddr_in6 destSockAddr6 = {};
            destSockAddr6.sin6_family = AF_INET6;
            destSockAddr6.sin6_addr = destIPv6;
            destSockAddr6.sin6_port = htons(destPort);

            ConnectionID connID(srcSockAddr6, destSockAddr6, Protocol::ICMPv6);
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