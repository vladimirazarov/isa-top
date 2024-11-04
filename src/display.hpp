/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#pragma once

#include "connectionsTable.hpp"
#include "connection.hpp"
#include "connectionID.hpp"
#include <ncurses.h>
#include <string>
#include <vector>
#include "display.hpp"
#include <chrono>
#include <thread>
#include <iomanip>
#include "connection.hpp"

// Display class handles showing the connections info on screen
class Display
{
public:
    // Constructor
    Display(ConnectionsTable &connectionsTable, SortBy sortBy, int updateInterval);
    // Destructor
    ~Display();

    void run();
    ConnectionsTable &m_connectionsTable;
    SortBy m_sortBy;
    int m_updateInterval;

    // Helper functions
    void printConnection(int row, Connection &connection);
    void init();
    void kill();
    void update();
    static std::string protocolToStr(Protocol protocol);
    std::string formatPacketRate(double packets);
    std::string formatTraffic(double bytes);
};
