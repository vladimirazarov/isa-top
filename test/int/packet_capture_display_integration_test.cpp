// test/int/packet_capture_display_integration_test.cpp

#include "../../src/packet.hpp"
#include "../../src/connectionsTable.hpp"
#include "../../src/connection.hpp"
#include "../../src/display.hpp"
#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <vector>
#include <netinet/ip.h>       
#include <netinet/tcp.h>      
#include <pcap.h>

TEST(PacketCaptureDisplayIntegrationTest, PacketCaptureAndDisplay) {
    ConnectionsTable connectionsTable;
    PacketCapture packetCapture("eth0", connectionsTable);

    in_addr localAddr;
    inet_pton(AF_INET, "192.168.1.10", &localAddr);
    packetCapture.m_localIPv4Addresses.push_back(localAddr);

    unsigned char packet[54]; 
    memset(packet, 0, sizeof(packet));

    struct ip* ipHeader = reinterpret_cast<struct ip*>(packet + 14);
    ipHeader->ip_v = 4;
    ipHeader->ip_hl = 5;
    ipHeader->ip_p = IPPROTO_TCP;
    inet_pton(AF_INET, "192.168.1.10", &(ipHeader->ip_src));
    inet_pton(AF_INET, "93.184.216.34", &(ipHeader->ip_dst));

    struct tcphdr* tcpHeader = reinterpret_cast<struct tcphdr*>(packet + 14 + (ipHeader->ip_hl * 4));
    tcpHeader->th_sport = htons(12345);
    tcpHeader->th_dport = htons(80);

    pcap_pkthdr pkthdr;
    pkthdr.caplen = sizeof(packet);
    pkthdr.len = sizeof(packet);

    PacketCapture::packetHandler(reinterpret_cast<unsigned char*>(&packetCapture), &pkthdr, packet);

    std::vector<Connection> connections;
    connectionsTable.getSortedConnections(SortBy::BY_BYTES, connections);

    ASSERT_EQ(connections.size(), 1);
    Connection conn = connections[0];

}
