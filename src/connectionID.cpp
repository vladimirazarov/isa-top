/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#include "connectionID.hpp"
#include <cstring>
#include <arpa/inet.h>
#include <sstream>

// Default constructor, initialize endpoints
ConnectionID::ConnectionID()
{
    m_protocol = Protocol::TCP;
    std::memset(&m_srcEndPoint, 0, sizeof(m_srcEndPoint));
    std::memset(&m_destEndPoint, 0, sizeof(m_destEndPoint));
}

// Constructor, set source, destination endpoints and protocol
ConnectionID::ConnectionID(const sockaddr_in6 &src, const sockaddr_in6 &dest, Protocol protocol)
{
    m_srcEndPoint = src;
    m_destEndPoint = dest;
    m_protocol = protocol;
}

// Stores IPv4 address in IPv6 structure
ConnectionID ConnectionID::storeIPv4InIPv6(const in_addr &src, uint16_t srcPort,
                                           const in_addr &dest, uint16_t destPort,
                                           Protocol protocol)
{
    sockaddr_in6 srcAddr6 = mapIPv4ToIPv6(src, srcPort);
    sockaddr_in6 destAddr6 = mapIPv4ToIPv6(dest, destPort);
    return ConnectionID(srcAddr6, destAddr6, protocol);
}

// Eq operator to compare 2 ConnectionID objects
bool ConnectionID::operator==(const ConnectionID &right) const
{
    return compareEndpoints(m_srcEndPoint, right.m_srcEndPoint) &&
           compareEndpoints(m_destEndPoint, right.m_destEndPoint) &&
           m_protocol == right.m_protocol;
}

// Maps IPv4 address and port to IPv6 structure
sockaddr_in6 ConnectionID::mapIPv4ToIPv6(const in_addr &ipv4Addr, uint16_t port)
{
    sockaddr_in6 ipv6Addr{};
    ipv6Addr.sin6_family = AF_INET6;
    ipv6Addr.sin6_port = htons(port);
    ipv6Addr.sin6_addr.s6_addr[10] = 0xFF;
    ipv6Addr.sin6_addr.s6_addr[11] = 0xFF;
    std::memcpy(&ipv6Addr.sin6_addr.s6_addr[12], &ipv4Addr, sizeof(ipv4Addr));
    return ipv6Addr;
}

// Compares two of sockaddr_in6 structures
bool ConnectionID::compareEndpoints(const sockaddr_in6 &ep1, const sockaddr_in6 &ep2)
{
    return std::memcmp(&ep1.sin6_addr, &ep2.sin6_addr, sizeof(in6_addr)) == 0 &&
           ep1.sin6_port == ep2.sin6_port;
}

// Getter for source endpoint
sockaddr_in6 ConnectionID::getSrcEndPoint() const
{
    return m_srcEndPoint;
}

// Getter for destination endpoint
sockaddr_in6 ConnectionID::getDestEndPoint() const
{
    return m_destEndPoint;
}

// Getter for protocol
Protocol ConnectionID::getProtocol() const
{
    return m_protocol;
}

// Getter for source port
uint16_t ConnectionID::getSrcPort() const
{
    return ntohs(m_srcEndPoint.sin6_port);
}

// Getter for destination port
uint16_t ConnectionID::getDestPort() const
{
    return ntohs(m_destEndPoint.sin6_port);
}

// Hash function for ConnectionID
std::size_t ConnectionIDHash::operator()(const ConnectionID &connection) const
{
    std::string srcStr = ConnectionID::endpointToString(connection.getSrcEndPoint());
    std::string destStr = ConnectionID::endpointToString(connection.getDestEndPoint());

    std::ostringstream oss;
    oss << srcStr << "-" << destStr << "-" << static_cast<int>(connection.getProtocol());
    std::string key = oss.str();

    return std::hash<std::string>{}(key);
}

// Method to convert sockaddr_in6 to string
std::string ConnectionID::endpointToString(const sockaddr_in6 &endpoint)
{
    char ipStr[INET6_ADDRSTRLEN];

    if (IN6_IS_ADDR_V4MAPPED(&endpoint.sin6_addr))
    {
        struct in_addr ipv4Addr;
        std::memcpy(&ipv4Addr, &endpoint.sin6_addr.s6_addr[12], sizeof(ipv4Addr));
        inet_ntop(AF_INET, &ipv4Addr, ipStr, sizeof(ipStr));
    }
    else
    {
        inet_ntop(AF_INET6, &endpoint.sin6_addr, ipStr, sizeof(ipStr));
    }

    uint16_t port = ntohs(endpoint.sin6_port);

    std::ostringstream oss;
    oss << ipStr << ":" << port;
    return oss.str();
}