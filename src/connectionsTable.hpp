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
#include "connection.hpp"
#include <iostream>



class ConnectionsTable
{
public:
    std::unordered_map<ConnectionID, Connection, ConnectionIDHash> connectionsTable;
    std::mutex tableMutex;

    void removeConnection(Connection connection);
    Connection *getConnection(Connection connection);
    // txOrRx: 1 - update tx (src)
    //         2 - update rx  (dst)
    void updateConnection(const ConnectionID &id, bool txRx, uint64_t bytes);
    void cleanupInactiveConnections(std::chrono::seconds timeout);


    void printConnections();

};