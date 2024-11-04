/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#pragma once

#include <pcap.h>
#include <string>
#include "connection.hpp"
#include "connectionsTable.hpp"

// PacketCapture class handles capturing and processing network packets
class PacketCapture
{
public:
    // Constuctor
    PacketCapture(std::string interfaceName, ConnectionsTable &connectionsTable);
    // Destructor
    ~PacketCapture();
    // Start and stop capture
    void startCapture();
    void stopCapture();
    // Static callback
    static void packetHandler(unsigned char *packetCaptureObject, const struct pcap_pkthdr *pkthdr, const unsigned char *packet);

    std::string m_interfaceName;
    uint m_linkLevelHeaderLen;
    int m_dataLinkType;
    pcap_t *m_pcapHandle;
    ConnectionsTable &m_connectionsTable;
    bool m_isCapturing;
    void initLocalAddresses();
    bool isLocalIPv4Address(const in_addr &address);
    bool isLocalIPv6Address(const in6_addr &address);
    // IPv4 local addresses
    std::vector<in_addr> m_localIPv4Addresses;
    // IPv6 local addresses
    std::vector<in6_addr> m_localIPv6Addresses;
};
