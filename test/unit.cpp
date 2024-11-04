#include "../src/cli.hpp"
#include "../src/connectionID.hpp"
#include "../src/connectionsTable.hpp"
#include "../src/connection.hpp"
#include "../src/packet.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <pcap.h>
#include <thread>
#include <chrono>
#include <ifaddrs.h>

std::vector<char*> createArgv(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr); 
    return argv;
}

sockaddr_in6 createIPv4MappedIPv6SockAddr(const std::string& ipv4Address, uint16_t port) {
    sockaddr_in6 addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    addr.sin6_addr.s6_addr[10] = 0xFF;
    addr.sin6_addr.s6_addr[11] = 0xFF;
    inet_pton(AF_INET, ipv4Address.c_str(), &addr.sin6_addr.s6_addr[12]);
    return addr;
}

sockaddr_in6 createIPv6SockAddr(const std::string& ipv6Address, uint16_t port) {
    sockaddr_in6 addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    inet_pton(AF_INET6, ipv6Address.c_str(), &addr.sin6_addr);
    return addr;
}

sockaddr_in6 createSockAddr6(uint32_t addr_part = 0) {
    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr.s6_addr32[3] = htonl(addr_part);
    return addr;
}

pcap_pkthdr createMockPcapHeader(uint32_t len) {
    pcap_pkthdr header;
    header.ts.tv_sec = 0;
    header.ts.tv_usec = 0;
    header.caplen = len;
    header.len = len;
    return header;
}

TEST(CommandLineInterfaceTest, ValidInterfaceOnly) {
    std::vector<std::string> args = {"program", "-i", "eth0"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    EXPECT_NO_THROW({
        CommandLineInterface cli(argc, argv.data());
        cli.validateRetrieveArgs();
    });
}

TEST(CommandLineInterfaceTest, MissingInterface) {
    std::vector<std::string> args = {"program", "-s", "b"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    EXPECT_EXIT({
        CommandLineInterface cli(argc, argv.data());
        cli.validateRetrieveArgs();
    }, ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST(ConnectionIDTest, DefaultConstructor) {
    ConnectionID connID;
    EXPECT_EQ(connID.getProtocol(), Protocol::TCP);

    sockaddr_in6 emptyAddr{};
    std::memset(&emptyAddr, 0, sizeof(emptyAddr));

    sockaddr_in6 srcEndPoint = connID.getSrcEndPoint();
    sockaddr_in6 destEndPoint = connID.getDestEndPoint();

    EXPECT_EQ(std::memcmp(&srcEndPoint, &emptyAddr, sizeof(sockaddr_in6)), 0);
    EXPECT_EQ(std::memcmp(&destEndPoint, &emptyAddr, sizeof(sockaddr_in6)), 0);
}

TEST(ConnectionsTableTest, GetSortedConnections_SortsByBytesCorrectly) {
    ConnectionsTable table;
    sockaddr_in6 src1 = createSockAddr6(1);
    sockaddr_in6 dest1 = createSockAddr6(2);
    ConnectionID id1(src1, dest1, Protocol::TCP);

    sockaddr_in6 src2 = createSockAddr6(3);
    sockaddr_in6 dest2 = createSockAddr6(4);
    ConnectionID id2(src2, dest2, Protocol::TCP);

    table.updateConnection(id1, true, 500);  
    table.updateConnection(id2, true, 1500); 

    std::vector<Connection> sortedConnections;
    table.getSortedConnections(SortBy::BY_BYTES, sortedConnections);

    ASSERT_EQ(sortedConnections.size(), 2);
    EXPECT_EQ(sortedConnections[0].m_ID, id2);
    EXPECT_EQ(sortedConnections[1].m_ID, id1);
}

TEST(PacketCaptureTest, IsLocalIPv4Address) {
    ConnectionsTable connectionsTable;
    PacketCapture packetCapture("eth0", connectionsTable);

    in_addr localAddr1;
    inet_pton(AF_INET, "192.168.1.10", &localAddr1);
    packetCapture.m_localIPv4Addresses.push_back(localAddr1);

    in_addr testAddr1;
    inet_pton(AF_INET, "192.168.1.10", &testAddr1);

    EXPECT_TRUE(packetCapture.isLocalIPv4Address(testAddr1));
}

TEST(PacketCaptureTest, PacketHandlerIPv4TCP) {
    ConnectionsTable connectionsTable;
    PacketCapture packetCapture("eth0", connectionsTable);

    packetCapture.m_dataLinkType = DLT_EN10MB; 
    packetCapture.m_linkLevelHeaderLen = 14;   

    in_addr localAddr;
    inet_pton(AF_INET, "192.168.1.10", &localAddr);
    packetCapture.m_localIPv4Addresses.push_back(localAddr);

    unsigned char packet[14 + sizeof(struct ip) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));

    struct ip* ipHeader = reinterpret_cast<struct ip*>(packet + 14);
    ipHeader->ip_v = 4;
    ipHeader->ip_hl = 5;
    ipHeader->ip_len = htons(sizeof(struct ip) + sizeof(struct tcphdr));
    ipHeader->ip_p = IPPROTO_TCP;
    inet_pton(AF_INET, "192.168.1.10", &(ipHeader->ip_src));
    inet_pton(AF_INET, "8.8.8.8", &(ipHeader->ip_dst));

    struct tcphdr* tcpHeader = reinterpret_cast<struct tcphdr*>(packet + 14 + (ipHeader->ip_hl * 4));
    tcpHeader->th_sport = htons(12345);
    tcpHeader->th_dport = htons(80);

    pcap_pkthdr pkthdr = createMockPcapHeader(sizeof(packet));

    PacketCapture::packetHandler(reinterpret_cast<unsigned char*>(&packetCapture), &pkthdr, packet);

    std::vector<Connection> connections;
    connectionsTable.getSortedConnections(SortBy::BY_BYTES, connections);

    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].m_bytesSent, pkthdr.len);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
