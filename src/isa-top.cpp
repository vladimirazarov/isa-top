/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#include "cli.hpp"
#include "connectionsTable.hpp"
#include "packet.hpp"
#include "display.hpp"
#include <csignal>
#include <thread>
#include <memory>
#include <iostream>

ConnectionsTable* globalConnectionsTable = nullptr;

void signalHandler(int signum) {
    endwin();
    std::cerr << "Interrupt signal (" << signum << ") received. Exiting..." << std::endl;
    exit(signum);
}

void runCapture(PacketCapture& pc) {
    pc.startCapture();
}

void runDisplay(Display& display) {
    display.run();
}

int main(int argc, char *argv[]) {
    CommandLineInterface cli(argc, argv);
    cli.validateRetrieveArgs();

    ConnectionsTable ct;
    globalConnectionsTable = &ct;

    if (!cli.m_logFilePath.empty()) {
        ct.setLogFilePath(cli.m_logFilePath);
    }

    std::signal(SIGINT, signalHandler);
    PacketCapture pc(cli.m_interface, ct);
    Display display(ct, cli.m_sortBy, 1);

    if (!cli.m_logFilePath.empty()) {
        ct.setLogFileStream();  
    }

    std::thread captureThread(runCapture, std::ref(pc));
    std::thread displayThread(runDisplay, std::ref(display));

    captureThread.join();
    displayThread.join();

    return 0;
}