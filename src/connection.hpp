/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#pragma once

#include <functional>
#include <string>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <sstream>
#include <arpa/inet.h>
#include <mutex>
#include "connectionID.hpp"

enum IPFamily
{
    IPv4,
    IPv6
};

// Connection class represents a network connection
class Connection
{
public:
    // ConnectionID is the unique connection identifier of a connection: srcIP + destIP + destPort + srcPort + Protocol for TCP/UDP
    //                                                                   srcIP + destIP + protocol                      for ICMP
    ConnectionID m_ID;
    // IP family (IPv4 or IPv6)
    IPFamily m_ipFamily;

    unsigned long int m_bytesSent;
    unsigned long int m_bytesReceived;
    unsigned long int m_packetsSent;
    unsigned long int m_packetsReceived;

    // Receive and transmit speeds (Bytes)
    double m_rxSpeedBytes;
    double m_txSpeedBytes;

    // Receive and transmit speeds (Packets)
    double m_rxSpeedPackets;
    double m_txSpeedPackets;

    std::chrono::system_clock::time_point m_first_seen;
    std::chrono::system_clock::time_point m_last_seen;
    // Constructors
    Connection(sockaddr_in6 srcEndPoint, sockaddr_in6 destEndPoint, IPFamily ipFamily, Protocol protocol);
    Connection();
};