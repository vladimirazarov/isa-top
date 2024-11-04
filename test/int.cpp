/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#include "../src/cli.hpp"
#include "../src/packet.hpp"
#include "../src/connectionsTable.hpp"
#include "../src/display.hpp"
#include "../src/connection.hpp"
#include "../src/connectionID.hpp"

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pcap.h>

// Helper function to convert a vector of strings to a vector of char* for argv
static std::vector<char*> createArgv(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);
    return argv;
}

// Helper to create a mock pcap header with a specific length
pcap_pkthdr createMockPcapHeader(uint32_t len) {
    pcap_pkthdr header;
    header.ts.tv_sec = 0;
    header.ts.tv_usec = 0;
    header.caplen = len;
    header.len = len;
    return header;
}

// Test to ensure CommandLineInterface initializes PacketCapture correctly
TEST(CommandLineInterfaceIntegrationTest, CLIInitializesPacketCapture) {
    std::vector<std::string> args = {"program", "-i", "eth0", "-s", "b"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    CommandLineInterface cli(argc, argv.data());
    cli.validateRetrieveArgs();

    ConnectionsTable connectionsTable;

    PacketCapture packetCapture(cli.m_interface, connectionsTable);

    EXPECT_EQ(packetCapture.m_interfaceName, "eth0");
}

// Test to simulate the main flow by capturing fake packets
TEST(FullSystemIntegrationTest, SimulatedMainFlow) {
    std::vector<std::string> args = {"program", "-i", "eth0", "-s", "p"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    CommandLineInterface cli(argc, argv.data());
    cli.validateRetrieveArgs();

    ConnectionsTable connectionsTable;

    PacketCapture packetCapture(cli.m_interface, connectionsTable);

    // Set data link type to Ethernet and header length
    packetCapture.m_dataLinkType = DLT_EN10MB; 
    packetCapture.m_linkLevelHeaderLen = 14;   

    // Add a local IPv4 address
    in_addr localAddr;
    inet_pton(AF_INET, "192.168.1.10", &localAddr);
    packetCapture.m_localIPv4Addresses.push_back(localAddr);

    // Simulate capturing 5 TCP packets
    for (int i = 0; i < 5; ++i) {
        unsigned char packet[14 + sizeof(struct ip) + sizeof(struct tcphdr)];
        memset(packet, 0, sizeof(packet));

        struct ip* ipHeader = reinterpret_cast<struct ip*>(packet + 14);
        ipHeader->ip_v = 4;
        ipHeader->ip_hl = 5;
        ipHeader->ip_len = htons(sizeof(struct ip) + sizeof(struct tcphdr));
        ipHeader->ip_p = IPPROTO_TCP;
        inet_pton(AF_INET, "192.168.1.10", &(ipHeader->ip_src));
        inet_pton(AF_INET, "93.184.216.34", &(ipHeader->ip_dst));

        struct tcphdr* tcpHeader = reinterpret_cast<struct tcphdr*>(packet + 14 + (ipHeader->ip_hl * 4));
        tcpHeader->th_sport = htons(12345 + i);
        tcpHeader->th_dport = htons(80);

        pcap_pkthdr pkthdr;
        pkthdr.caplen = sizeof(packet);
        pkthdr.len = sizeof(packet);

        PacketCapture::packetHandler(reinterpret_cast<unsigned char*>(&packetCapture), &pkthdr, packet);
    }

    std::vector<Connection> connections;
    connectionsTable.getSortedConnections(cli.m_sortBy, connections);

    ASSERT_EQ(connections.size(), 5);
}

// Test to check integration between PacketCapture and Display
TEST(PacketCaptureDisplayIntegrationTest, PacketCaptureAndDisplay) {
    ConnectionsTable connectionsTable;
    PacketCapture packetCapture("eth0", connectionsTable);

    // Set data link type to Ethernet and header length
    packetCapture.m_dataLinkType = DLT_EN10MB; 
    packetCapture.m_linkLevelHeaderLen = 14;   

    // Add a local IPv4 address
    in_addr localAddr;
    inet_pton(AF_INET, "192.168.1.10", &localAddr);
    packetCapture.m_localIPv4Addresses.push_back(localAddr);

    Display display(connectionsTable, SortBy::BY_BYTES, 1);

    display.init();
    display.update();

    SUCCEED(); 
}

// Test to ensure PacketCapture correctly updates the ConnectionsTable
TEST(PacketCaptureIntegrationTest, PacketCaptureUpdatesConnectionsTable) {
    ConnectionsTable connectionsTable;
    PacketCapture packetCapture("eth0", connectionsTable);

    // Set data link type to Ethernet and header length
    packetCapture.m_dataLinkType = DLT_EN10MB; 
    packetCapture.m_linkLevelHeaderLen = 14;   

    // Add a local IPv4 address
    in_addr localAddr;
    inet_pton(AF_INET, "192.168.1.10", &localAddr);
    packetCapture.m_localIPv4Addresses.push_back(localAddr);

    // Create a fake TCP packet
    unsigned char packet[14 + sizeof(struct ip) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));

    struct ip* ipHeader = reinterpret_cast<struct ip*>(packet + 14);
    ipHeader->ip_v = 4;
    ipHeader->ip_hl = 5;
    ipHeader->ip_len = htons(sizeof(struct ip) + sizeof(struct tcphdr));
    ipHeader->ip_p = IPPROTO_TCP;
    inet_pton(AF_INET, "192.168.1.10", &(ipHeader->ip_src));
    inet_pton(AF_INET, "93.184.216.34", &(ipHeader->ip_dst));

    struct tcphdr* tcpHeader = reinterpret_cast<struct tcphdr*>(
        packet + 14 + (ipHeader->ip_hl * 4));
    tcpHeader->th_sport = htons(12345);
    tcpHeader->th_dport = htons(80);

    pcap_pkthdr pkthdr = createMockPcapHeader(sizeof(packet));

    // Process the fake packet
    PacketCapture::packetHandler(reinterpret_cast<unsigned char*>(&packetCapture), &pkthdr, packet);

    // Create expected ConnectionID
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