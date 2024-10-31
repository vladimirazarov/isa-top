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
    mvprintw(0, 0, "%-25s %-25s %-8s %-18s %-18s",
             "Src IP:Port", "Dst IP:Port", "Proto", "Rx", "Tx");
    mvprintw(1, 0, "%-25s %-25s %-8s %-9s %-8s %-9s %-8s",
             "", "", "", "b/s", "p/s", "b/s", "p/s");
    mvhline(2, 0, '-', maxC);
    m_connectionsTable.calculateSpeed();

    std::vector<Connection> connections;
    m_connectionsTable.getSortedConnections(m_sortBy, connections);
    m_connectionsTable.getTopConnections(10, connections);

    m_connectionsTable.logConnectionsTable(m_sortBy);

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
}

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
             formatPacketRate(connection.m_txSpeedPackets).c_str()
    );
}

std::string Display::formatTraffic(double bytes)
{
    const char *units[] = {"B", "K", "M", "G", "T"};
    double size = bytes;
    int unitIndex = 0;

    if (size < 1 && size > 0)
    {
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

std::string Display::formatPacketRate(double packets)
{
    const char *units[] = {"", "K", "M", "G", "T"};
    double rate = packets;
    int unitIndex = 0;

    while (rate >= 1000 && unitIndex < 4)
    {
        rate /= 1000;
        unitIndex++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << rate << units[unitIndex];
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