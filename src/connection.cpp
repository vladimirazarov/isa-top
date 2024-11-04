/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#include "connection.hpp"
#include "connectionID.hpp"

// Constructor
Connection::Connection(sockaddr_in6 srcEndPoint, sockaddr_in6 destEndpoint, IPFamily ipv4oripv6, Protocol protocol)
{
    // Source endpoint
    m_ID.m_srcEndPoint = srcEndPoint;
    // Destination endpoint
    m_ID.m_destEndPoint = destEndpoint;
    // Protocol
    m_ID.m_protocol = protocol;
    // IP Family
    m_ipFamily = ipv4oripv6;
    // Bytes count initializatoin
    m_bytesSent = m_bytesReceived = m_packetsSent = m_packetsReceived = 0;
    // First seen time
    m_first_seen = std::chrono::system_clock::now();
    // Last seen time
    m_last_seen = std::chrono::system_clock::now();
};

// Default consctructor
Connection::Connection()
{
    // Initialize required attributes
    m_ipFamily = IPv4;
    m_bytesSent = m_bytesReceived = m_packetsSent = m_packetsReceived = 0;
    m_rxSpeedBytes = m_txSpeedBytes = m_rxSpeedPackets = m_txSpeedPackets = 0;
    m_first_seen = std::chrono::system_clock::now();
    m_last_seen = std::chrono::system_clock::now();
};
