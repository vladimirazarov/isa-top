/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#pragma once
#include <netinet/in.h>
#include <string>

enum class Protocol {
    TCP,
    UDP,
    ICMP,
    ICMPv6
};

class ConnectionID {
public:
    ConnectionID();
    ConnectionID(const sockaddr_in6& src, const sockaddr_in6& dest, Protocol protocol);

    static ConnectionID storeIPv4InIPv6(const in_addr& src, uint16_t srcPort,
                                       const in_addr& dest, uint16_t destPort,
                                       Protocol protocol);

    bool operator==(const ConnectionID& right) const;

    sockaddr_in6 getSrcEndPoint() const;
    sockaddr_in6 getDestEndPoint() const;
    Protocol getProtocol() const;
    uint16_t getSrcPort() const;
    uint16_t getDestPort() const;
    static sockaddr_in6 mapIPv4ToIPv6(const in_addr& ipv4Addr, uint16_t port);
    static bool compareEndpoints(const sockaddr_in6& ep1, const sockaddr_in6& ep2);
    static std::string endpointToString(const sockaddr_in6& endpoint);

    sockaddr_in6 m_srcEndPoint;
    sockaddr_in6 m_destEndPoint;
    Protocol m_protocol;
    uint16_t m_srcPort;
    uint16_t m_destPort;

    
};

struct ConnectionIDHash {
    std::size_t operator()(const ConnectionID& connection) const;
};

