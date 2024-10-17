#include "../../src/connectionID.hpp"
#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>

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

TEST(ConnectionIDTest, ParameterizedConstructorIPv6) {
    sockaddr_in6 src = createIPv6SockAddr("2001:db8::1", 12345);
    sockaddr_in6 dest = createIPv6SockAddr("2001:db8::2", 54321);
    ConnectionID connID(src, dest, Protocol::UDP);

    EXPECT_EQ(connID.getProtocol(), Protocol::UDP);

    sockaddr_in6 srcEndPoint = connID.getSrcEndPoint();
    sockaddr_in6 destEndPoint = connID.getDestEndPoint();

    EXPECT_EQ(std::memcmp(&srcEndPoint, &src, sizeof(sockaddr_in6)), 0);
    EXPECT_EQ(std::memcmp(&destEndPoint, &dest, sizeof(sockaddr_in6)), 0);
}

TEST(ConnectionIDTest, ParameterizedConstructorIPv4Mapped) {
    sockaddr_in6 src = createIPv4MappedIPv6SockAddr("192.168.1.1", 12345);
    sockaddr_in6 dest = createIPv4MappedIPv6SockAddr("192.168.1.2", 54321);
    ConnectionID connID(src, dest, Protocol::TCP);

    EXPECT_EQ(connID.getProtocol(), Protocol::TCP);

    sockaddr_in6 srcEndPoint = connID.getSrcEndPoint();
    sockaddr_in6 destEndPoint = connID.getDestEndPoint();

    EXPECT_EQ(std::memcmp(&srcEndPoint, &src, sizeof(sockaddr_in6)), 0);
    EXPECT_EQ(std::memcmp(&destEndPoint, &dest, sizeof(sockaddr_in6)), 0);
}

TEST(ConnectionIDTest, StoreIPv4InIPv6) {
    in_addr srcIPv4Addr, destIPv4Addr;
    inet_pton(AF_INET, "192.168.1.1", &srcIPv4Addr);
    inet_pton(AF_INET, "192.168.1.2", &destIPv4Addr);
    uint16_t srcPort = 12345;
    uint16_t destPort = 54321;

    ConnectionID connID = ConnectionID::storeIPv4InIPv6(srcIPv4Addr, srcPort, destIPv4Addr, destPort, Protocol::TCP);

    sockaddr_in6 expectedSrc = createIPv4MappedIPv6SockAddr("192.168.1.1", srcPort);
    sockaddr_in6 expectedDest = createIPv4MappedIPv6SockAddr("192.168.1.2", destPort);

    EXPECT_EQ(connID.getProtocol(), Protocol::TCP);

    sockaddr_in6 srcEndPoint = connID.getSrcEndPoint();
    sockaddr_in6 destEndPoint = connID.getDestEndPoint();

    EXPECT_EQ(std::memcmp(&srcEndPoint, &expectedSrc, sizeof(sockaddr_in6)), 0);
    EXPECT_EQ(std::memcmp(&destEndPoint, &expectedDest, sizeof(sockaddr_in6)), 0);
}

TEST(ConnectionIDTest, EqualityOperator) {
    sockaddr_in6 src1 = createIPv6SockAddr("2001:db8::1", 12345);
    sockaddr_in6 dest1 = createIPv6SockAddr("2001:db8::2", 54321);
    ConnectionID connID1(src1, dest1, Protocol::TCP);

    sockaddr_in6 src2 = createIPv6SockAddr("2001:db8::1", 12345);
    sockaddr_in6 dest2 = createIPv6SockAddr("2001:db8::2", 54321);
    ConnectionID connID2(src2, dest2, Protocol::TCP);

    EXPECT_TRUE(connID1 == connID2);

    ConnectionID connID3(src2, dest2, Protocol::UDP);
    EXPECT_FALSE(connID1 == connID3);

    sockaddr_in6 srcDiffPort = createIPv6SockAddr("2001:db8::1", 12346);
    ConnectionID connID4(srcDiffPort, dest2, Protocol::TCP);
    EXPECT_FALSE(connID1 == connID4);

    sockaddr_in6 destDiffAddr = createIPv6SockAddr("2001:db8::3", 54321);
    ConnectionID connID5(src2, destDiffAddr, Protocol::TCP);
    EXPECT_FALSE(connID1 == connID5);
}

TEST(ConnectionIDTest, HashFunction) {
    sockaddr_in6 src1 = createIPv6SockAddr("2001:db8::1", 12345);
    sockaddr_in6 dest1 = createIPv6SockAddr("2001:db8::2", 54321);
    ConnectionID connID1(src1, dest1, Protocol::TCP);

    sockaddr_in6 src2 = createIPv6SockAddr("2001:db8::1", 12345);
    sockaddr_in6 dest2 = createIPv6SockAddr("2001:db8::2", 54321);
    ConnectionID connID2(src2, dest2, Protocol::TCP);

    ConnectionIDHash hashFunc;
    size_t hash1 = hashFunc(connID1);
    size_t hash2 = hashFunc(connID2);

    EXPECT_EQ(hash1, hash2);

    sockaddr_in6 srcDiff = createIPv6SockAddr("2001:db8::3", 12345);
    ConnectionID connID3(srcDiff, dest2, Protocol::TCP);
    size_t hash3 = hashFunc(connID3);

    EXPECT_NE(hash1, hash3);
}

TEST(ConnectionIDTest, GetPorts) {
    sockaddr_in6 src = createIPv6SockAddr("2001:db8::1", 12345);
    sockaddr_in6 dest = createIPv6SockAddr("2001:db8::2", 54321);
    ConnectionID connID(src, dest, Protocol::TCP);

    EXPECT_EQ(connID.getSrcPort(), 12345);
    EXPECT_EQ(connID.getDestPort(), 54321);
}

TEST(ConnectionIDTest, EndpointToString) {
    sockaddr_in6 endpointIPv6 = createIPv6SockAddr("2001:db8::1", 12345);
    std::string endpointStrIPv6 = ConnectionID::endpointToString(endpointIPv6);
    EXPECT_EQ(endpointStrIPv6, "2001:db8::1:12345");

    sockaddr_in6 endpointIPv4Mapped = createIPv4MappedIPv6SockAddr("192.168.1.1", 54321);
    std::string endpointStrIPv4Mapped = ConnectionID::endpointToString(endpointIPv4Mapped);
    EXPECT_EQ(endpointStrIPv4Mapped, "192.168.1.1:54321");
}

TEST(ConnectionIDTest, CompareEndpoints) {
    sockaddr_in6 ep1 = createIPv6SockAddr("2001:db8::1", 12345);
    sockaddr_in6 ep2 = createIPv6SockAddr("2001:db8::1", 12345);
    sockaddr_in6 ep3 = createIPv6SockAddr("2001:db8::2", 12345);
    sockaddr_in6 ep4 = createIPv6SockAddr("2001:db8::1", 54321);

    EXPECT_TRUE(ConnectionID::compareEndpoints(ep1, ep2));
    EXPECT_FALSE(ConnectionID::compareEndpoints(ep1, ep3));  
    EXPECT_FALSE(ConnectionID::compareEndpoints(ep1, ep4));  
}

TEST(ConnectionIDTest, MapIPv4ToIPv6) {
    in_addr ipv4Addr;
    inet_pton(AF_INET, "192.168.1.1", &ipv4Addr);
    uint16_t port = 12345;

    sockaddr_in6 mappedAddr = ConnectionID::mapIPv4ToIPv6(ipv4Addr, port);

    sockaddr_in6 expectedAddr = createIPv4MappedIPv6SockAddr("192.168.1.1", port);
    EXPECT_EQ(std::memcmp(&mappedAddr, &expectedAddr, sizeof(sockaddr_in6)), 0);
}

TEST(ConnectionIDTest, GetEndpoints) {
    sockaddr_in6 src = createIPv6SockAddr("2001:db8::1", 12345);
    sockaddr_in6 dest = createIPv6SockAddr("2001:db8::2", 54321);
    ConnectionID connID(src, dest, Protocol::TCP);

    sockaddr_in6 srcEndPoint = connID.getSrcEndPoint();
    sockaddr_in6 destEndPoint = connID.getDestEndPoint();

    EXPECT_EQ(std::memcmp(&srcEndPoint, &src, sizeof(sockaddr_in6)), 0);
    EXPECT_EQ(std::memcmp(&destEndPoint, &dest, sizeof(sockaddr_in6)), 0);
}

TEST(ConnectionIDTest, GetProtocol) {
    sockaddr_in6 src = createIPv6SockAddr("2001:db8::1", 12345);
    sockaddr_in6 dest = createIPv6SockAddr("2001:db8::2", 54321);
    ConnectionID connID(src, dest, Protocol::UDP);

    EXPECT_EQ(connID.getProtocol(), Protocol::UDP);
}
