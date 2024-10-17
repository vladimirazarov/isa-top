#include "display.hpp"

Display::Display(ConnectionsTable &connectionsTable, SortBy sortBy, int updateInterval) : m_connectionsTable(connectionsTable)
{
    m_sortBy = sortBy;
    m_updateInterval = updateInterval;
};
Display::~Display()
{
    endwin();
}
void Display::init()
{
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

}
void Display::update()
{
    int maxR, maxC;
    getmaxyx(stdscr, maxR, maxC);

    clear();
    // Print header
    mvprintw(0, 0, "%-30s %-30s %-8s %-15s %-15s %-15s %-15s",
             "Src IP:Port", "Dst IP:Port", "Proto", "Rx (b/s)", "Rx (p/s)", "Tx (b/s)", "Tx (p/s)");
    mvhline(1, 0, '-', 120);
    m_connectionsTable.calculateSpeed();

    std::vector<Connection> connections;
    m_connectionsTable.getSortedConnections(m_sortBy, connections);
    m_connectionsTable.getTopConnections(10, connections);
    int row = 2;
    for (auto current = connections.begin(); current != connections.end(); current++)
    {
        printConnection(row++, *current);
    }
    refresh();
}

std::string Display::protocolToStr(Protocol protocol)
{
    switch (protocol)
    {
    case Protocol::TCP:
        return "tcp";
        break;

    case Protocol::UDP:
        return "udp";
        break;
    case Protocol::ICMP:
        return "icmp";
        break;
    case Protocol::ICMPv6:
        return "icmpv6";
        break;
    }
}
void Display::printConnection(int row, Connection &connection)
{
    std::string srcIPfull = ConnectionID::endpointToString(connection.m_ID.getSrcEndPoint());
    std::string destIPfull = ConnectionID::endpointToString(connection.m_ID.getDestEndPoint());
    std::string protocol;

    mvprintw(row + 1, 0, "%-30s %-30s %-8s %-15s %-15s %-15s %-15s",
             srcIPfull.c_str(),
             destIPfull.c_str(),

             protocolToStr(connection.m_ID.m_protocol).c_str(),

             formatTraffic(connection.m_rxSpeedBytes).c_str(),
             formatTraffic(connection.m_rxSpeedPackets).c_str(),
             formatTraffic(connection.m_txSpeedBytes).c_str(),
             formatTraffic(connection.m_txSpeedPackets).c_str()
    );
}


std::string Display::formatTraffic(double bytes)
{
    const char *units[] = {"B", "K", "M", "G", "T"};
    double size = bytes;
    int unitIndex = 0;

    if (size < 1 && size > 0) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << "B";  
        return oss.str();
    }

    while (size >= 1024 && unitIndex < 4)
    {
        size /= 1024;
        unitIndex++;
    }


    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << units[unitIndex];
    return oss.str();
}

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