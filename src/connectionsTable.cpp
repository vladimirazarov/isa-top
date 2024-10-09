#include "connectionsTable.hpp"
#include "connectionID.hpp"

void ConnectionsTable::removeConnection(Connection connection)
{
    std::lock_guard<std::mutex> lock(tableMutex);
    connectionsTable.erase(connection.m_ID);
}

Connection *ConnectionsTable::getConnection(Connection connection)
{
    std::lock_guard<std::mutex> lock(tableMutex);
    auto foundConnection = connectionsTable.find(connection.m_ID);
    if (foundConnection != connectionsTable.end())
    {
        return &(foundConnection->second);
    }
    return nullptr;
}

void ConnectionsTable::updateConnection(const ConnectionID& id, bool isSending, uint64_t byteCount)
{
    std::lock_guard<std::mutex> lock(tableMutex);

    auto currentConnection = connectionsTable.find(id);

    if (currentConnection != connectionsTable.end())
    {
        Connection& connection = currentConnection->second;
        connection.m_last_seen = std::chrono::system_clock::now();

        if (isSending)
        {
            connection.m_bytesSent += byteCount;
            connection.m_packetsSent += 1;
        }
        else
        {
            connection.m_bytesReceived += byteCount;
            connection.m_packetsReceived += 1;
        }
    }
    else
    {
        Connection newConnection;
        newConnection.m_ID = id;

        if (id.getSrcEndPoint().sin6_family == AF_INET6)
        {
            newConnection.m_ipFamily = IPFamily::IPv6;
        }
        else
        {
            newConnection.m_ipFamily = IPFamily::IPv4;
        }

        auto now = std::chrono::system_clock::now();
        newConnection.m_first_seen = now;
        newConnection.m_last_seen = now;

        if (isSending)
        {
            newConnection.m_bytesSent = byteCount;
            newConnection.m_packetsSent = 1;
            newConnection.m_bytesReceived = 0;
            newConnection.m_packetsReceived = 0;
        }
        else
        {
            newConnection.m_bytesSent = 0;
            newConnection.m_packetsSent = 0;
            newConnection.m_bytesReceived = byteCount;
            newConnection.m_packetsReceived = 1;
        }

        connectionsTable.insert({id, newConnection});
    }
}

void ConnectionsTable::cleanupInactiveConnections(std::chrono::seconds timeout)
{
    std::lock_guard<std::mutex> lock(tableMutex);
    auto now = std::chrono::system_clock::now();

    for (auto currentConnection = connectionsTable.begin(); currentConnection != connectionsTable.end(); )
    {
        if (now - currentConnection->second.m_last_seen > timeout)
        {
            currentConnection = connectionsTable.erase(currentConnection);
        }
        else
        {
            ++currentConnection;
        }
    }
}


// TODO: DELETE LATER
// TODO: FIX IPv4 and IPv6 not displayed
void ConnectionsTable::printConnections() {
    std::lock_guard<std::mutex> lock(tableMutex);
    for (const auto& pair : connectionsTable) {
        std::cout << "Connection: " 
                  << pair.first.getSrcEndPoint().sin6_addr.s6_addr << ":" << pair.first.getSrcPort()
                  << " -> " 
                  << pair.first.getDestEndPoint().sin6_addr.s6_addr << ":" << pair.first.getDestPort()
                  << " Protocol: " << static_cast<int>(pair.first.getProtocol())
                  << " Bytes Transmitted: " << pair.second.m_bytesSent
                  << " Packets Transmitted: " << pair.second.m_packetsSent
                  << " Bytes Received: " << pair.second.m_bytesReceived
                  << " Packets Received: " << pair.second.m_packetsReceived
                  << std::endl;
    }
}

