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
// ConnectionID is the unique connection identifier of a connection: srcIP + destIP + destPort + srcPort + Protocol for TCP/UDP
//                                                                   srcIP + destIP + protocol                      for ICMP


class Connection
{
public:
    ConnectionID m_ID;
    IPFamily m_ipFamily;

    unsigned long int m_bytesSent;
    unsigned long int m_bytesReceived;
    unsigned long int m_packetsSent;
    unsigned long int m_packetsReceived;

    unsigned long int m_bytesSentBefore;
    unsigned long int m_bytesReceivedBefore;

    double m_txSpeedBytes;
    double m_rxSpeedBytes;


    std::chrono::system_clock::time_point m_first_seen;
    std::chrono::system_clock::time_point m_last_seen;

    Connection(sockaddr_in6 srcEndPoint, sockaddr_in6 destEndPoint, IPFamily ipFamily, Protocol protocol);
    Connection();
};