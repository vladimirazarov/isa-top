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

enum SortBy{
    BY_BYTES, 
    BY_PACKETS
};


class ConnectionsTable
{
public:
    // Connection table 1 sec before now
    std::unordered_map<ConnectionID, Connection, ConnectionIDHash> m_connectionsTableBefore;
    // Connection table right now
    std::unordered_map<ConnectionID, Connection, ConnectionIDHash> m_connectionsTable;
    std::mutex m_tableMutex;

    void removeConnection(Connection connection);
    Connection *getConnection(Connection connection);

    // txOrRx: 1 - update tx (src)
    //         2 - update rx  (dst)
    void updateConnection(const ConnectionID &id, bool txRx, uint64_t bytes);
    void calculateSpeed();

    void getSortedConnections(SortBy sortBy, std::vector<Connection> &outputVector);
    void getTopConnections(int num, std::vector <Connection> &connectionsSorted);

};