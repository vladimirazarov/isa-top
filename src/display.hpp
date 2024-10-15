#pragma once

#include "connectionsTable.hpp"
#include "connection.hpp"
#include "connectionID.hpp"
#include <ncurses.h>
#include <string>

class Display{
    public:
        Display(ConnectionsTable &connectionsTable, SortBy sortBy, int updateInterval);
        ~Display();

        void run();
    private:
        ConnectionsTable &m_connectionsTable;
        SortBy m_sortBy;
        int m_updateInterval;
        void printConnection(int row, Connection &connection);
        void init();
        void kill();
        void update();
        std::string protocolToStr(Protocol protocol);
};
