#include "connectionsTable.hpp"
#include "connectionID.hpp"

void ConnectionsTable::removeConnection(Connection connection)
{
    std::lock_guard<std::mutex> lock(m_tableMutex);
    m_connectionsTable.erase(connection.m_ID);
}

Connection *ConnectionsTable::getConnection(Connection connection)
{
    std::lock_guard<std::mutex> lock(m_tableMutex);
    auto foundConnection = m_connectionsTable.find(connection.m_ID);
    if (foundConnection != m_connectionsTable.end())
    {
        return &(foundConnection->second);
    }
    return nullptr;
}

void ConnectionsTable::updateConnection(const ConnectionID &id, bool isSending, uint64_t byteCount)
{
    std::lock_guard<std::mutex> lock(m_tableMutex);

    auto currentConnection = m_connectionsTable.find(id);

    if (currentConnection != m_connectionsTable.end())
    {
        Connection &connection = currentConnection->second;
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

        m_connectionsTable.insert({id, newConnection});
    }
}

void ConnectionsTable::cleanupInactiveConnections(std::chrono::seconds timeout)
{
    std::lock_guard<std::mutex> lock(m_tableMutex);
    auto now = std::chrono::system_clock::now();

    for (auto currentConnection = m_connectionsTable.begin(); currentConnection != m_connectionsTable.end();)
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

void ConnectionsTable::printConnections(SortBy sortBy)
{
    std::lock_guard<std::mutex> lock(m_tableMutex);
    for (const auto &pair : m_connectionsTable)
    {
        std::cout << "Connection: "
                  << pair.first.endpointToString(pair.first.m_srcEndPoint) << ":" << pair.first.getSrcPort()
                  << " -> "
                  << pair.first.endpointToString(pair.first.m_destEndPoint) << ":" << pair.first.getDestPort()
                  << " Protocol: " << static_cast<int>(pair.first.getProtocol())
                  << " Bytes Transmitted: " << pair.second.m_bytesSent
                  << " Packets Transmitted: " << pair.second.m_packetsSent
                  << " Bytes Received: " << pair.second.m_bytesReceived
                  << " Packets Received: " << pair.second.m_packetsReceived
                  << std::endl;
    }
}

void ConnectionsTable::calculateSpeed()
{
    std::lock_guard<std::mutex> lock(m_tableMutex);
    for (auto &pair : m_connectionsTable)
    {
        Connection &current = pair.second;

        current.m_rxSpeedBytes = static_cast<double>(current.m_bytesReceived - current.m_bytesReceivedBefore);
        current.m_txSpeedBytes = static_cast<double>(current.m_bytesSent - current.m_bytesSentBefore);

        current.m_bytesSentBefore = current.m_bytesSent;
        current.m_bytesReceivedBefore = current.m_bytesReceived;
    }
}
