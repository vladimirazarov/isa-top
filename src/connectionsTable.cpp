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
        const ConnectionID &connectionID = pair.first; 
        Connection &current = pair.second;

        auto before = m_connectionsTableBefore.find(connectionID);  
        if (before != m_connectionsTableBefore.end())
        {
        Connection &connectionBefore = before->second;

        current.m_rxSpeedBytes = static_cast<double>(current.m_bytesReceived - connectionBefore.m_bytesReceived);
        current.m_txSpeedBytes = static_cast<double>(current.m_bytesSent - connectionBefore.m_bytesSent);

        current.m_rxSpeedPackets= static_cast<double>(current.m_packetsReceived- connectionBefore.m_packetsReceived);
        current.m_txSpeedPackets= static_cast<double>(current.m_packetsSent- connectionBefore.m_packetsSent);

        }
        else
        {
            current.m_rxSpeedBytes = 0;
            current.m_txSpeedBytes = 0;

            current.m_rxSpeedPackets = 0;
            current.m_txSpeedPackets = 0;
        }

        m_connectionsTableBefore[connectionID] = current;
    }

    for (auto current = m_connectionsTableBefore.begin(); current != m_connectionsTableBefore.end(); current++) {
        if (m_connectionsTable.find(current->first) == m_connectionsTable.end()) {
            current = m_connectionsTableBefore.erase(current);
        } else {
            current++;
        }
    }
}


std::vector<Connection> ConnectionsTable::getSortedConnections(SortBy sortBy, std::vector<Connection> &outputVector)
{
    std::lock_guard<std::mutex> lock(m_tableMutex);
    std::vector<Connection> connectionsTableSorted;

    // Sort by bytes
    if (sortBy = SortBy::BY_BYTES)
    {
        std::sort(m_connectionsTable.begin(), m_connectionsTable.end(), []( const std::pair<Connection, ConnectionID> &first, 
                                                                            const std::pair<Connection, ConnectionID> &second){
                                                                            return      (first.first.m_bytesReceived + first.first.m_bytesSent) 
                                                                                    <   (second.first.m_bytesReceived + second.first.m_bytesSent);
                                                                        });
    }

    // Sort by packets
    if (sortBy = SortBy::BY_PACKETS)
    {
        std::sort(m_connectionsTable.begin(), m_connectionsTable.end(), []( const std::pair<Connection, ConnectionID> &first, 
                                                                            const std::pair<Connection, ConnectionID> &second){
                                                                            return      (first.first.m_packetsReceived+ first.first.m_packetsSent) 
                                                                                    <   (second.first.m_packetsReceived+ second.first.m_packetsSent);
                                                                        });
    }

    // Convert set to vector
    for (auto current = m_connectionsTable.begin(); current != m_connectionsTable.end(); current++)
    {
        outputVector.push_back(m_connectionsTable.find(current->first)->second);

    }
    return outputVector;
}

