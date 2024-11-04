/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#include "display.hpp"

// Constructor
Display::Display(ConnectionsTable &connectionsTable, SortBy sortBy, int updateInterval) : m_connectionsTable(connectionsTable)
{
    m_sortBy = sortBy;
    m_updateInterval = updateInterval;
};

// Desctructor
Display::~Display()
{
    endwin();
}

// Initialize ncurses
void Display::init()
{
    // Ncurses mode
    initscr();
    // Disable line buffering
    cbreak();
    // Don't show what user types
    noecho();
    // Hide the cursor
    curs_set(0);
    // Non-blocking
    nodelay(stdscr, TRUE);
    // Enable special keys
    keypad(stdscr, TRUE);
}

// Updates display every 1 second
void Display::update()
{
    int maxC;
    // Screen size
    getmaxyx(stdscr, maxC, maxC);

    clear();
    // Header
    mvprintw(0, 0, "%-25s %-25s %-8s %-18s %-18s",
             "Src IP:Port", "Dst IP:Port", "Proto", "Rx", "Tx");
    mvprintw(1, 0, "%-25s %-25s %-8s %-9s %-8s %-9s %-8s",
             "", "", "", "b/s", "p/s", "b/s", "p/s");
    // Separator
    mvhline(2, 0, '-', maxC);
    // Update speeds
    m_connectionsTable.calculateSpeed();

    // Create vector of Connection objects (in order to retreive connections that will be displayed)
    std::vector<Connection> connections;
    // Sort connections
    m_connectionsTable.getSortedConnections(m_sortBy, connections);
    // Only show top 10 connections
    m_connectionsTable.getTopConnections(10, connections);
    // Log connections table (if --log was specified)
    m_connectionsTable.logConnectionsTable(m_sortBy);

    // Print each connection
    int row = 2;
    for (auto current = connections.begin(); current != connections.end(); current++)
    {
        printConnection(row++, *current);
    }

    refresh();
}

// Method to convert protocol enum to string
std::string Display::protocolToStr(Protocol protocol)
{
    switch (protocol)
    {
    case Protocol::TCP:
        return "TCP";
        break;

    case Protocol::UDP:
        return "UDP";
        break;
    case Protocol::ICMP:
        return "ICMP";
        break;
    case Protocol::ICMPv6:
        return "ICMPv6";
        break;
    }
    return "Unknown";
}

// Print a single connection on the specific row
void Display::printConnection(int row, Connection &connection)
{
    std::string srcIPfull = ConnectionID::endpointToString(connection.m_ID.getSrcEndPoint());
    std::string destIPfull = ConnectionID::endpointToString(connection.m_ID.getDestEndPoint());

    mvprintw(row + 2, 0, "%-25s %-25s %-8s %-9s %-8s %-9s %-8s",
             srcIPfull.c_str(),
             destIPfull.c_str(),
             protocolToStr(connection.m_ID.m_protocol).c_str(),
             formatTraffic(connection.m_rxSpeedBytes).c_str(),
             formatPacketRate(connection.m_rxSpeedPackets).c_str(),
             formatTraffic(connection.m_txSpeedBytes).c_str(),
             formatPacketRate(connection.m_txSpeedPackets).c_str());
}

// Format bytes into readable format
std::string Display::formatTraffic(double bytes)
{
    const char *units[] = {"B", "K", "M", "G", "T"};
    double size = bytes;
    int unitIndex = 0;

    // Change unitIndex if speed is high
    while (size >= 1024 && unitIndex < 4)
    {
        size /= 1024;
        unitIndex++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << units[unitIndex];
    return oss.str();
}

// Format packets into readable format
std::string Display::formatPacketRate(double packets)
{
    const char *units[] = {"", "K", "M", "G", "T"};
    double rate = packets;
    int unitIndex = 0;

    // Change unitIndex if speed is high
    while (rate >= 1000 && unitIndex < 4)
    {
        rate /= 1000;
        unitIndex++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << rate << units[unitIndex];
    return oss.str();
}

// Main loop
void Display::run()
{
    init();

    while (true)
    {
        update();

        std::this_thread::sleep_for(std::chrono::seconds(m_updateInterval));
    }

    endwin();
}