#include "../../src/connectionsTable.hpp"
#include "../../src/connectionID.hpp"
#include "../../src/connection.hpp" 
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <chrono>
#include <thread>
#include <vector>

sockaddr_in6 createSockAddr6(uint32_t addr_part = 0)
{
    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr.s6_addr32[3] = htonl(addr_part);
    return addr;
}

TEST(ConnectionsTableTest, GetSortedConnections_SortsByBytesCorrectly)
{
    ConnectionsTable table;
    sockaddr_in6 src1 = createSockAddr6(1);
    sockaddr_in6 dest1 = createSockAddr6(2);
    ConnectionID id1(src1, dest1, Protocol::TCP);

    sockaddr_in6 src2 = createSockAddr6(3);
    sockaddr_in6 dest2 = createSockAddr6(4);
    ConnectionID id2(src2, dest2, Protocol::TCP);

    sockaddr_in6 src3 = createSockAddr6(5);
    sockaddr_in6 dest3 = createSockAddr6(6);
    ConnectionID id3(src3, dest3, Protocol::TCP);

    table.updateConnection(id1, true, 500);  
    table.updateConnection(id2, true, 1500); 
    table.updateConnection(id3, true, 1000); 

    std::vector<Connection> sortedConnections;
    table.getSortedConnections(SortBy::BY_BYTES, sortedConnections);

    ASSERT_EQ(sortedConnections.size(), 3);
    EXPECT_EQ(sortedConnections[0].m_ID, id2); 
    EXPECT_EQ(sortedConnections[1].m_ID, id3); 
    EXPECT_EQ(sortedConnections[2].m_ID, id1); 
}

TEST(ConnectionsTableTest, GetSortedConnections_SortsByPacketsCorrectly)
{
    ConnectionsTable table;

    sockaddr_in6 src1 = createSockAddr6(1);
    sockaddr_in6 dest1 = createSockAddr6(2);
    ConnectionID id1(src1, dest1, Protocol::TCP);

    sockaddr_in6 src2 = createSockAddr6(3);
    sockaddr_in6 dest2 = createSockAddr6(4);
    ConnectionID id2(src2, dest2, Protocol::TCP);

    sockaddr_in6 src3 = createSockAddr6(5);
    sockaddr_in6 dest3 = createSockAddr6(6);
    ConnectionID id3(src3, dest3, Protocol::TCP);

    table.updateConnection(id1, true, 100); 
    table.updateConnection(id1, true, 200); 

    table.updateConnection(id2, true, 300); 

    table.updateConnection(id3, true, 400); 
    table.updateConnection(id3, true, 500); 
    table.updateConnection(id3, true, 600); 

    std::vector<Connection> sortedConnections;
    table.getSortedConnections(SortBy::BY_PACKETS, sortedConnections);

    ASSERT_EQ(sortedConnections.size(), 3);
    EXPECT_EQ(sortedConnections[0].m_ID, id3); 
    EXPECT_EQ(sortedConnections[1].m_ID, id1); 
    EXPECT_EQ(sortedConnections[2].m_ID, id2); 
}

TEST(ConnectionsTableTest, GetTopConnections_ReturnsCorrectNumber)
{
    ConnectionsTable table;

    sockaddr_in6 src1 = createSockAddr6(1);
    sockaddr_in6 dest1 = createSockAddr6(2);
    ConnectionID id1(src1, dest1, Protocol::TCP);

    sockaddr_in6 src2 = createSockAddr6(3);
    sockaddr_in6 dest2 = createSockAddr6(4);
    ConnectionID id2(src2, dest2, Protocol::TCP);

    sockaddr_in6 src3 = createSockAddr6(5);
    sockaddr_in6 dest3 = createSockAddr6(6);
    ConnectionID id3(src3, dest3, Protocol::TCP);

    table.updateConnection(id1, true, 500);
    table.updateConnection(id2, true, 1000);
    table.updateConnection(id3, true, 1500);

    std::vector<Connection> sortedConnections;
    table.getSortedConnections(SortBy::BY_BYTES, sortedConnections);
    table.getTopConnections(2, sortedConnections);

    ASSERT_EQ(sortedConnections.size(), 2);
    EXPECT_EQ(sortedConnections[0].m_ID, id3); 
    EXPECT_EQ(sortedConnections[1].m_ID, id2); 
}

TEST(ConnectionsTableTest, GetTopConnections_WithLargeNum)
{
    ConnectionsTable table;

    sockaddr_in6 src = createSockAddr6(1);
    sockaddr_in6 dest = createSockAddr6(2);
    ConnectionID id(src, dest, Protocol::TCP);
    table.updateConnection(id, true, 500);

    std::vector<Connection> sortedConnections;
    table.getSortedConnections(SortBy::BY_BYTES, sortedConnections);

    table.getTopConnections(5, sortedConnections);

    ASSERT_EQ(sortedConnections.size(), 1);
    EXPECT_EQ(sortedConnections[0].m_ID, id);
}

TEST(ConnectionsTableTest, UpdateConnection_UpdatesExistingConnection)
{
    ConnectionsTable table;

    sockaddr_in6 src = createSockAddr6(1);
    sockaddr_in6 dest = createSockAddr6(2);
    ConnectionID id(src, dest, Protocol::TCP);

    table.updateConnection(id, true, 500);
    table.updateConnection(id, false, 300);

    Connection tempConnection;
    tempConnection.m_ID = id;

    Connection *connection = table.getConnection(tempConnection);
    ASSERT_NE(connection, nullptr);
    EXPECT_EQ(connection->m_bytesSent, 500);
    EXPECT_EQ(connection->m_bytesReceived, 300);
    EXPECT_EQ(connection->m_packetsSent, 1);
    EXPECT_EQ(connection->m_packetsReceived, 1);
}

TEST(ConnectionsTableTest, RemoveConnection_NonExistentConnection)
{
    ConnectionsTable table;

    sockaddr_in6 src = createSockAddr6(1);
    sockaddr_in6 dest = createSockAddr6(2);
    ConnectionID id(src, dest, Protocol::TCP);

    Connection tempConnection;
    tempConnection.m_ID = id;
    table.removeConnection(tempConnection);

    std::vector<Connection> connections;
    table.getSortedConnections(SortBy::BY_BYTES, connections);
    EXPECT_EQ(connections.size(), 0);
}

TEST(ConnectionsTableTest, CalculateSpeed_NoPreviousState)
{
    ConnectionsTable table;

    sockaddr_in6 src = createSockAddr6(1);
    sockaddr_in6 dest = createSockAddr6(2);
    ConnectionID id(src, dest, Protocol::TCP);

    table.updateConnection(id, true, 500);

    table.calculateSpeed();

    Connection tempConnection;
    tempConnection.m_ID = id;

    Connection *connection = table.getConnection(tempConnection);
    ASSERT_NE(connection, nullptr);
    EXPECT_EQ(connection->m_txSpeedBytes, 0);
    EXPECT_EQ(connection->m_rxSpeedBytes, 0);
}

TEST(ConnectionsTableTest, GetSortedConnections_EmptyTable)
{
    ConnectionsTable table;

    std::vector<Connection> sortedConnections;
    table.getSortedConnections(SortBy::BY_BYTES, sortedConnections);

    EXPECT_EQ(sortedConnections.size(), 0);
}
