/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#include "connection.hpp"
#include "connectionID.hpp"



Connection::Connection(sockaddr_in6 srcEndPoint, sockaddr_in6 destEndpoint, IPFamily ipv4oripv6, Protocol protocol)
{
    m_ID.m_srcEndPoint = srcEndPoint;
    m_ID.m_destEndPoint = destEndpoint;
    m_ID.m_protocol = protocol;
    m_ipFamily = ipv4oripv6;
    m_bytesSent = m_bytesReceived = m_packetsSent = m_packetsReceived = 0;
    m_first_seen = std::chrono::system_clock::now();
    m_last_seen = std::chrono::system_clock::now();
};
Connection::Connection()
{
    m_ipFamily = IPv4;
    m_bytesSent = m_bytesReceived = m_packetsSent = m_packetsReceived = 0;
    m_rxSpeedBytes= m_txSpeedBytes= m_rxSpeedPackets= m_txSpeedPackets = 0;
    m_first_seen = std::chrono::system_clock::now();
    m_last_seen = std::chrono::system_clock::now();
};
