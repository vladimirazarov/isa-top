/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#pragma once

#include <functional>
#include <algorithm>
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
#include "connection.hpp"
#include <iostream>
#include <memory>
#include <fstream>

// Enum to specify sorting criteria
enum SortBy
{
    BY_BYTES,
    BY_PACKETS
};

// Class to manage all network connections
class ConnectionsTable
{
public:
    // Connections table 1 sec before now
    std::unordered_map<ConnectionID, Connection, ConnectionIDHash> m_connectionsTableBefore;
    // Connections table right now
    std::unordered_map<ConnectionID, Connection, ConnectionIDHash> m_connectionsTable;
    // Mutex for thread safety
    std::mutex m_tableMutex;

    void removeConnection(Connection connection);
    Connection *getConnection(Connection connection);

    // txOrRx: 1 - update tx (src)
    //         2 - update rx  (dst)
    void updateConnection(const ConnectionID &id, bool txRx, uint64_t bytes);
    void calculateSpeed();

    void getSortedConnections(SortBy sortBy, std::vector<Connection> &outputVector);
    void getTopConnections(unsigned int num, std::vector<Connection> &connectionsSorted);

    void parseEndpoint(const std::string &endpoint, std::string &ip, std::string &port);
    void setLogFileStream();
    void logConnectionsTable(SortBy sortBy);
    std::shared_ptr<std::ofstream> m_logFileStream;
    std::string m_logFilePath;
    std::mutex m_logMutex;

    void setLogFilePath(const std::string &logFilePath);
};