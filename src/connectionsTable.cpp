#include "connectionsTable.hpp"
#include "connectionID.hpp"
#include "display.hpp"

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
    logConnectionUpdate(id);
}

void ConnectionsTable::calculateSpeed()
{
    std::lock_guard<std::mutex> lock(m_tableMutex);
    auto now = std::chrono::system_clock::now();

    for (auto &pair : m_connectionsTable)
    {
        const ConnectionID &connectionID = pair.first;
        Connection &current = pair.second;

        // Find the previous state
        auto before = m_connectionsTableBefore.find(connectionID);
        if (before != m_connectionsTableBefore.end())
        {
            Connection &connectionBefore = before->second;

            double timeDeltaSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - connectionBefore.m_last_seen).count();
            if (timeDeltaSeconds > 0)
            {
                current.m_rxSpeedBytes = (current.m_bytesReceived - connectionBefore.m_bytesReceived) / timeDeltaSeconds;
                current.m_txSpeedBytes = (current.m_bytesSent - connectionBefore.m_bytesSent) / timeDeltaSeconds;

                current.m_rxSpeedPackets = (current.m_packetsReceived - connectionBefore.m_packetsReceived) / timeDeltaSeconds;
                current.m_txSpeedPackets = (current.m_packetsSent - connectionBefore.m_packetsSent) / timeDeltaSeconds;
            }
            else
            {
                current.m_rxSpeedBytes = 0;
                current.m_txSpeedBytes = 0;
                current.m_rxSpeedPackets = 0;
                current.m_txSpeedPackets = 0;
            }
        }
        else
        {
            current.m_rxSpeedBytes = 0;
            current.m_txSpeedBytes = 0;
            current.m_rxSpeedPackets = 0;
            current.m_txSpeedPackets = 0;
        }

        // Update the previous state to the current
        current.m_last_seen = now;
        m_connectionsTableBefore[connectionID] = current;
    }

    // Clean up stale connections in m_connectionsTableBefore
    for (auto current = m_connectionsTableBefore.begin(); current != m_connectionsTableBefore.end();)
    {
        if (m_connectionsTable.find(current->first) == m_connectionsTable.end())
        {
            current = m_connectionsTableBefore.erase(current);
        }
        else
        {
            current++;
        }
    }
}

void ConnectionsTable::getSortedConnections(SortBy sortBy, std::vector<Connection> &outputVector)
{
    std::lock_guard<std::mutex> lock(m_tableMutex);
    std::vector<std::pair<ConnectionID, Connection>> connections(m_connectionsTable.begin(), m_connectionsTable.end());

    if (sortBy == SortBy::BY_BYTES)
    {
        std::sort(connections.begin(), connections.end(), [](const std::pair<ConnectionID, Connection> &first, const std::pair<ConnectionID, Connection> &second)
                  { return (first.second.m_bytesReceived + first.second.m_bytesSent) > (second.second.m_bytesReceived + second.second.m_bytesSent); });
    }
    else if (sortBy == SortBy::BY_PACKETS)
    {
        std::sort(connections.begin(), connections.end(), [](const std::pair<ConnectionID, Connection> &first, const std::pair<ConnectionID, Connection> &second)
                  { return (first.second.m_packetsReceived + first.second.m_packetsSent) > (second.second.m_packetsReceived + second.second.m_packetsSent); });
    }

    outputVector.clear();
    for (auto current = connections.begin(); current != connections.end(); current++)
    {
        outputVector.push_back(current->second);
    }
}

void ConnectionsTable::getTopConnections(unsigned int num, std::vector<Connection> &connectionsSorted)
{
    if (num > connectionsSorted.size())
    {
        return;
    }

    connectionsSorted.resize(num);
}

void ConnectionsTable::setLogFileStream(std::shared_ptr<std::ofstream> logFileStream) {
    m_logFileStream = logFileStream;

    if (m_logFileStream && m_logFileStream->is_open()) {
        std::lock_guard<std::mutex> lock(m_logMutex);
        *m_logFileStream << "timestamp,protocol,src_ip,src_port,dst_ip,dst_port,bytes_sent,bytes_received,packets_sent,packets_received\n";
    }
}

void ConnectionsTable::logConnectionUpdate(const ConnectionID &id) {
    if (m_logFileStream && m_logFileStream->is_open()) {
        std::lock_guard<std::mutex> lock(m_logMutex);

        const Connection &connection = m_connectionsTable[id];
        auto timestamp = std::chrono::system_clock::to_time_t(connection.m_last_seen);

        std::string srcEndpoint = ConnectionID::endpointToString(connection.m_ID.getSrcEndPoint());
        std::string destEndpoint = ConnectionID::endpointToString(connection.m_ID.getDestEndPoint());
        std::string protocol = Display::protocolToStr(connection.m_ID.getProtocol());

        std::string srcIP, srcPortStr, destIP, destPortStr;
        parseEndpoint(srcEndpoint, srcIP, srcPortStr);
        parseEndpoint(destEndpoint, destIP, destPortStr);

        uint16_t srcPort = std::stoi(srcPortStr);
        uint16_t destPort = std::stoi(destPortStr);

        *m_logFileStream << timestamp << ","
                         << protocol << ","
                         << srcIP << ","
                         << srcPort << ","
                         << destIP << ","
                         << destPort << ","
                         << connection.m_bytesSent << ","
                         << connection.m_bytesReceived << ","
                         << connection.m_packetsSent << ","
                         << connection.m_packetsReceived << "\n";
    m_logFileStream->flush();
    }
}

void ConnectionsTable::parseEndpoint(const std::string &endpoint, std::string &ip, std::string &port) {
    size_t colonPos = endpoint.find_last_of(':');
    if (colonPos != std::string::npos) {
        ip = endpoint.substr(0, colonPos);
        port = endpoint.substr(colonPos + 1);
    } else {
        ip = endpoint;
        port = "0";
    }
}