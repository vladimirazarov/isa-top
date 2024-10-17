#include "../../src/packet.hpp"
#include "../../src/connectionsTable.hpp"
#include "../../src/connectionID.hpp"
#include "../../src/connection.hpp"
#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <pcap.h>
#include <netinet/ip.h>       
#include <netinet/ip6.h>      
#include <netinet/tcp.h>      
#include <netinet/udp.h>      
#include <netinet/ip_icmp.h>  
#include <netinet/icmp6.h>    

pcap_pkthdr createMockPcapHeader(uint32_t len) {
    pcap_pkthdr header;
    header.ts.tv_sec = 0;
    header.ts.tv_usec = 0;
    header.caplen = len;
    header.len = len;
    return header;
}

TEST(PacketCaptureTest, IsLocalIPv4Address) {
    ConnectionsTable connectionsTable;
    PacketCapture packetCapture("eth0", connectionsTable);

    in_addr localAddr1, localAddr2;
    inet_pton(AF_INET, "192.168.1.10", &localAddr1);
    inet_pton(AF_INET, "10.0.0.5", &localAddr2);

    packetCapture.m_localIPv4Addresses.push_back(localAddr1);
    packetCapture.m_localIPv4Addresses.push_back(localAddr2);

    in_addr testAddr1, testAddr2, testAddr3;
    inet_pton(AF_INET, "192.168.1.10", &testAddr1);
    inet_pton(AF_INET, "10.0.0.5", &testAddr2);
    inet_pton(AF_INET, "172.16.0.1", &testAddr3);

    EXPECT_TRUE(packetCapture.isLocalIPv4Address(testAddr1));
    EXPECT_TRUE(packetCapture.isLocalIPv4Address(testAddr2));
    EXPECT_FALSE(packetCapture.isLocalIPv4Address(testAddr3));
}

TEST(PacketCaptureTest, IsLocalIPv6Address) {
    ConnectionsTable connectionsTable;
    PacketCapture packetCapture("eth0", connectionsTable);

    in6_addr localAddr1, localAddr2;
    inet_pton(AF_INET6, "fe80::1", &localAddr1);
    inet_pton(AF_INET6, "2001:db8::5", &localAddr2);

    packetCapture.m_localIPv6Addresses.push_back(localAddr1);
    packetCapture.m_localIPv6Addresses.push_back(localAddr2);

    in6_addr testAddr1, testAddr2, testAddr3;
    inet_pton(AF_INET6, "fe80::1", &testAddr1);
    inet_pton(AF_INET6, "2001:db8::5", &testAddr2);
    inet_pton(AF_INET6, "2001:db8::6", &testAddr3);

    EXPECT_TRUE(packetCapture.isLocalIPv6Address(testAddr1));
    EXPECT_TRUE(packetCapture.isLocalIPv6Address(testAddr2));
    EXPECT_FALSE(packetCapture.isLocalIPv6Address(testAddr3));
}

TEST(PacketCaptureTest, PacketHandlerIPv4TCP) {
    ConnectionsTable connectionsTable;
    PacketCapture packetCapture("eth0", connectionsTable);

    in_addr localAddr;
    inet_pton(AF_INET, "192.168.1.10", &localAddr);
    packetCapture.m_localIPv4Addresses.push_back(localAddr);

    unsigned char packet[54]; 
    memset(packet, 0, sizeof(packet));

    struct ip *ipHeader = reinterpret_cast<struct ip *>(packet + 14);
    ipHeader->ip_v = 4;
    ipHeader->ip_hl = 5;
    ipHeader->ip_p = IPPROTO_TCP;
    inet_pton(AF_INET, "192.168.1.10", &(ipHeader->ip_src));
    inet_pton(AF_INET, "8.8.8.8", &(ipHeader->ip_dst));

    struct tcphdr *tcpHeader = reinterpret_cast<struct tcphdr *>(packet + 14 + (ipHeader->ip_hl * 4));
    tcpHeader->th_sport = htons(12345);
    tcpHeader->th_dport = htons(80);

    pcap_pkthdr pkthdr = createMockPcapHeader(sizeof(packet));

    PacketCapture::packetHandler(reinterpret_cast<unsigned char *>(&packetCapture), &pkthdr, packet);

    sockaddr_in6 srcSockAddr6 = ConnectionID::mapIPv4ToIPv6(ipHeader->ip_src, ntohs(tcpHeader->th_sport));
    sockaddr_in6 destSockAddr6 = ConnectionID::mapIPv4ToIPv6(ipHeader->ip_dst, ntohs(tcpHeader->th_dport));
    ConnectionID expectedConnID(srcSockAddr6, destSockAddr6, Protocol::TCP);

    std::vector<Connection> connections;
    connectionsTable.getSortedConnections(SortBy::BY_BYTES, connections);

    ASSERT_EQ(connections.size(), 1);
    Connection conn = connections[0];

    EXPECT_TRUE(conn.m_ID == expectedConnID);
    EXPECT_EQ(conn.m_bytesSent, pkthdr.len);
    EXPECT_EQ(conn.m_bytesReceived, 0);
}

TEST(PacketCaptureTest, PacketHandlerIPv6UDP) {
    ConnectionsTable connectionsTable;
    PacketCapture packetCapture("eth0", connectionsTable);

    in6_addr localAddr;
    inet_pton(AF_INET6, "2001:db8::1", &localAddr);
    packetCapture.m_localIPv6Addresses.push_back(localAddr);

    unsigned char packet[74];
    memset(packet, 0, sizeof(packet));

    struct ip6_hdr *ip6Header = reinterpret_cast<struct ip6_hdr *>(packet + 14);
    ip6Header->ip6_flow = htonl((6 << 28)); 
    ip6Header->ip6_nxt = IPPROTO_UDP;
    inet_pton(AF_INET6, "2001:db8::1", &(ip6Header->ip6_src));
    inet_pton(AF_INET6, "2001:db8::2", &(ip6Header->ip6_dst));

    struct udphdr *udpHeader = reinterpret_cast<struct udphdr *>(packet + 14 + sizeof(struct ip6_hdr));
    udpHeader->uh_sport = htons(12345);
    udpHeader->uh_dport = htons(53);

    pcap_pkthdr pkthdr = createMockPcapHeader(sizeof(packet));

    PacketCapture::packetHandler(reinterpret_cast<unsigned char *>(&packetCapture), &pkthdr, packet);

    sockaddr_in6 srcSockAddr6 = {};
    srcSockAddr6.sin6_family = AF_INET6;
    srcSockAddr6.sin6_addr = ip6Header->ip6_src;
    srcSockAddr6.sin6_port = udpHeader->uh_sport;

    sockaddr_in6 destSockAddr6 = {};
    destSockAddr6.sin6_family = AF_INET6;
    destSockAddr6.sin6_addr = ip6Header->ip6_dst;
    destSockAddr6.sin6_port = udpHeader->uh_dport;

    ConnectionID expectedConnID(srcSockAddr6, destSockAddr6, Protocol::UDP);

    std::vector<Connection> connections;
    connectionsTable.getSortedConnections(SortBy::BY_BYTES, connections);

    ASSERT_EQ(connections.size(), 1);
    Connection conn = connections[0];

    EXPECT_TRUE(conn.m_ID == expectedConnID);
    EXPECT_EQ(conn.m_bytesSent, pkthdr.len);
    EXPECT_EQ(conn.m_bytesReceived, 0);
}

TEST(PacketCaptureTest, PacketHandlerICMP) {
    ConnectionsTable connectionsTable;
    PacketCapture packetCapture("eth0", connectionsTable);

    in_addr localAddr;
    inet_pton(AF_INET, "192.168.1.10", &localAddr);
    packetCapture.m_localIPv4Addresses.push_back(localAddr);

    unsigned char packet[42]; 
    memset(packet, 0, sizeof(packet));

    struct ip *ipHeader = reinterpret_cast<struct ip *>(packet + 14);
    ipHeader->ip_v = 4;
    ipHeader->ip_hl = 5;
    ipHeader->ip_p = IPPROTO_ICMP;
    inet_pton(AF_INET, "192.168.1.10", &(ipHeader->ip_src));
    inet_pton(AF_INET, "8.8.8.8", &(ipHeader->ip_dst));

    struct icmphdr *icmpHeader = reinterpret_cast<struct icmphdr *>(packet + 14 + (ipHeader->ip_hl * 4));
    icmpHeader->type = ICMP_ECHO;

    pcap_pkthdr pkthdr = createMockPcapHeader(sizeof(packet));

    PacketCapture::packetHandler(reinterpret_cast<unsigned char *>(&packetCapture), &pkthdr, packet);

    sockaddr_in6 srcSockAddr6 = ConnectionID::mapIPv4ToIPv6(ipHeader->ip_src, 0);
    sockaddr_in6 destSockAddr6 = ConnectionID::mapIPv4ToIPv6(ipHeader->ip_dst, 0);
    ConnectionID expectedConnID(srcSockAddr6, destSockAddr6, Protocol::ICMP);

    std::vector<Connection> connections;
    connectionsTable.getSortedConnections(SortBy::BY_BYTES, connections);

    ASSERT_EQ(connections.size(), 1);
    Connection conn = connections[0];

    EXPECT_TRUE(conn.m_ID == expectedConnID);
    EXPECT_EQ(conn.m_bytesSent, pkthdr.len);
    EXPECT_EQ(conn.m_bytesReceived, 0);
}
