#include "../../src/cli.hpp"
#include "../../src/packet.hpp"
#include "../../src/connectionsTable.hpp"
#include "../../src/display.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <netinet/ip.h>       
#include <netinet/tcp.h>      
#include <pcap.h>

static std::vector<char*> createArgv(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr); 
    return argv;
}

TEST(FullSystemIntegrationTest, SimulatedMainFlow) {
    std::vector<std::string> args = {"program", "-i", "eth0", "-s", "p"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    CommandLineInterface cli(argc, argv.data());
    cli.validateRetrieveArgs();

    ConnectionsTable connectionsTable;

    PacketCapture packetCapture(cli.m_interface, connectionsTable);

    in_addr localAddr;
    inet_pton(AF_INET, "192.168.1.10", &localAddr);
    packetCapture.m_localIPv4Addresses.push_back(localAddr);

    for (int i = 0; i < 5; ++i) {
        unsigned char packet[54];
        memset(packet, 0, sizeof(packet));

        struct ip* ipHeader = reinterpret_cast<struct ip*>(packet + 14);
        ipHeader->ip_v = 4;
        ipHeader->ip_hl = 5;
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
