/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#include "connectionsTable.hpp"
#include "connectionID.hpp"
#include "display.hpp"

// Erase connection from the table
void ConnectionsTable::removeConnection(Connection connection)
{
    // Lock the table and erase
    std::lock_guard<std::mutex> lock(m_tableMutex);
    m_connectionsTable.erase(connection.m_ID);
}

// Get pointer to a connection, return nullptr if it doesn't exist
Connection *ConnectionsTable::getConnection(Connection connection)
{
    // Lock the table
    std::lock_guard<std::mutex> lock(m_tableMutex);
    // Find connection
    auto foundConnection = m_connectionsTable.find(connection.m_ID);
    if (foundConnection != m_connectionsTable.end())
    {
        return &(foundConnection->second);
    }
    // Not found
    return nullptr;
}

// Updates connection depending on what it does (sends or receives)
void ConnectionsTable::updateConnection(const ConnectionID &id, bool isSending, uint64_t byteCount)
{
    // Lock the table
    std::lock_guard<std::mutex> lock(m_tableMutex);
    // Find connection
    auto currentConnection = m_connectionsTable.find(id);

    if (currentConnection != m_connectionsTable.end())
    {
        Connection &connection = currentConnection->second;
        connection.m_last_seen = std::chrono::system_clock::now();
        // Sending -> increment bytes and packets sent
        if (isSending)
        {
            connection.m_bytesSent += byteCount;
            connection.m_packetsSent += 1;
        }
        // Receiving -> increment bytes and packets received
        else
        {
            connection.m_bytesReceived += byteCount;
            connection.m_packetsReceived += 1;
        }
    }
    // Otherwise its new connection. Create new Connection object
    else
    {
        // Initialization
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

// Calculates speed of transfer for all connections that are currently in the connection table
// To achieve this, ConnectionTableBefore is used. It represents ConnectionsTableState 1 second ago
void ConnectionsTable::calculateSpeed()
{
    // Lock the table
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
            // Connection is active
            if (timeDeltaSeconds > 0)
            {
                // Speed calculation
                current.m_rxSpeedBytes = (current.m_bytesReceived - connectionBefore.m_bytesReceived) / timeDeltaSeconds;
                current.m_txSpeedBytes = (current.m_bytesSent - connectionBefore.m_bytesSent) / timeDeltaSeconds;

                current.m_rxSpeedPackets = (current.m_packetsReceived - connectionBefore.m_packetsReceived) / timeDeltaSeconds;
                current.m_txSpeedPackets = (current.m_packetsSent - connectionBefore.m_packetsSent) / timeDeltaSeconds;
            }
            // Connection is inactive, set speeds to 0
            else
            {
                current.m_rxSpeedBytes = 0;
                current.m_txSpeedBytes = 0;
                current.m_rxSpeedPackets = 0;
                current.m_txSpeedPackets = 0;
            }
        }
        // No previous data at all, set speeds to 0
        else
        {
            current.m_rxSpeedBytes = 0;
            current.m_txSpeedBytes = 0;
            current.m_rxSpeedPackets = 0;
            current.m_txSpeedPackets = 0;
        }

        // Update connectionsTableBefore and last seen
        current.m_last_seen = now;
        m_connectionsTableBefore[connectionID] = current;
    }
    // Clean up connectionsTableBefore
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

// Sorts connections either by bytes or by packets and returns sorted connections represented as list (vector)
void ConnectionsTable::getSortedConnections(SortBy sortBy, std::vector<Connection> &outputVector)
{
    // Lock the table
    std::lock_guard<std::mutex> lock(m_tableMutex);

    std::vector<std::pair<ConnectionID, Connection>> connections(m_connectionsTable.begin(), m_connectionsTable.end());

    // Use std::sort to get connections sorted by bytes
    if (sortBy == SortBy::BY_BYTES)
    {
        std::sort(connections.begin(), connections.end(), [](const std::pair<ConnectionID, Connection> &first, const std::pair<ConnectionID, Connection> &second)
                  { return (first.second.m_bytesReceived + first.second.m_bytesSent) > (second.second.m_bytesReceived + second.second.m_bytesSent); });
    }
    // Use std::sort to get connections sorted by packets
    else if (sortBy == SortBy::BY_PACKETS)
    {
        std::sort(connections.begin(), connections.end(), [](const std::pair<ConnectionID, Connection> &first, const std::pair<ConnectionID, Connection> &second)
                  { return (first.second.m_packetsReceived + first.second.m_packetsSent) > (second.second.m_packetsReceived + second.second.m_packetsSent); });
    }

    // Fill up output vector
    outputVector.clear();
    for (const auto &current : connections)
    {
        outputVector.push_back(current.second);
    }
}

// Gets top connections (truncates the vector from getSortedConnections)
void ConnectionsTable::getTopConnections(unsigned int num, std::vector<Connection> &connectionsSorted)
{
    if (num > connectionsSorted.size())
    {
        return;
    }

    connectionsSorted.resize(num);
}

// Function needed for logging. Sets file stream if -l is specified, otherwise do nothing
void ConnectionsTable::setLogFileStream()
{
    if (!m_logFilePath.empty())
    {
        // Lock the log
        std::lock_guard<std::mutex> lock(m_logMutex);
        // Initialize stream
        m_logFileStream = std::make_shared<std::ofstream>(m_logFilePath, std::ios::out | std::ios::trunc);
        // Ensure stream is opened
        if (!m_logFileStream->is_open())
        {
            exit(EXIT_FAILURE);
        }
        // If stream is opened, make header
        else
        {
            *m_logFileStream << "timestamp,protocol,src_ip,src_port,dst_ip,dst_port,bytes_sent,bytes_received,packets_sent,packets_received\n";
            m_logFileStream->flush();
        }
    }
}
// Extract ip and port from endpoint string
void ConnectionsTable::parseEndpoint(const std::string &endpoint, std::string &ip, std::string &port)
{
    // Find port location
    size_t colonPos = endpoint.find_last_of(':');
    // Slice into 2 part based on port location
    if (colonPos != std::string::npos)
    {
        ip = endpoint.substr(0, colonPos);
        port = endpoint.substr(colonPos + 1);
    }
    // No colon, set entire string as ip representation
    else
    {
        ip = endpoint;
        port = "0";
    }
}

// Logs the whole connections table into log
void ConnectionsTable::logConnectionsTable(SortBy sortBy)
{
    if (!m_logFilePath.empty())
    {
        // Lock the log
        std::lock_guard<std::mutex> lock(m_logMutex);
        // Get file stream
        m_logFileStream = std::make_shared<std::ofstream>(m_logFilePath, std::ios::trunc | std::ios::out);

        if (!m_logFileStream->is_open())
        {
            return;
        }

        // Update header
        *m_logFileStream << "timestamp,protocol,src_ip,src_port,dst_ip,dst_port,bytes_sent,bytes_received,packets_sent,packets_received\n";

        std::vector<Connection> connections;
        getSortedConnections(sortBy, connections);
        // Uncomment the next line to store only top 10 connections into log file
        //  getTopConnections(10, connections);

        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        // Fill up log the file with all connections that exist in connectionsTable
        for (const Connection &connection : connections)
        {
            std::string srcEndpoint = ConnectionID::endpointToString(connection.m_ID.getSrcEndPoint());
            std::string destEndpoint = ConnectionID::endpointToString(connection.m_ID.getDestEndPoint());
            std::string protocol = Display::protocolToStr(connection.m_ID.getProtocol());

            std::string srcIP, srcPortStr, destIP, destPortStr;
            parseEndpoint(srcEndpoint, srcIP, srcPortStr);
            parseEndpoint(destEndpoint, destIP, destPortStr);

            uint16_t srcPort = std::stoi(srcPortStr);
            uint16_t destPort = std::stoi(destPortStr);

            // Make one line
            *m_logFileStream << now << ","
                             << protocol << ","
                             << srcIP << ","
                             << srcPort << ","
                             << destIP << ","
                             << destPort << ","
                             << connection.m_bytesSent << ","
                             << connection.m_bytesReceived << ","
                             << connection.m_packetsSent << ","
                             << connection.m_packetsReceived << "\n";
        }

        m_logFileStream->flush();
    }
}

// Helper function to set log file path
void ConnectionsTable::setLogFilePath(const std::string &logFilePath)
{
    m_logFilePath = logFilePath;
}