#include <iostream>
#include <threads.h>
#include "display.hpp"
#include <vector>
#include <string>
#include "connection.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "packet.hpp"
#include "connectionsTable.hpp"
#include <csignal>
#include "cli.hpp"
#include <thread>

ConnectionsTable* globalConnectionsTable = nullptr;

void signalHandler(int signum) {
    //TODO: call destr
    endwin();
    exit(signum);
}


void runCapture(PacketCapture& pc) {
    pc.startCapture();
}

void runDisplay(Display& display, int updateInterval) {
    display.run();
}


int main(int argc, char *argv[]) {
    CommandLineInterface cli(argc, argv);
    cli.validateRetrieveArgs();

    ConnectionsTable ct;
    globalConnectionsTable = &ct;

    std::signal(SIGINT, signalHandler);

    PacketCapture pc(cli.m_interface, *globalConnectionsTable);
    Display display(*globalConnectionsTable, cli.m_sortBy, 1); 

    std::thread captureThread(runCapture, std::ref(pc));
    std::thread displayThread(runDisplay, std::ref(display), 1);

    captureThread.join();
    displayThread.join();

    return 0;
}