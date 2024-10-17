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
#include <fstream>
#include <memory>

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

    std::shared_ptr<std::ofstream> logFileStream;

    if (!cli.m_logFilePath.empty()) {
        logFileStream = std::make_shared<std::ofstream>(cli.m_logFilePath, std::ios::out);
        if (!logFileStream->is_open()) {
            std::cerr << "Error opening log file: " << cli.m_logFilePath << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    PacketCapture pc(cli.m_interface, ct);
    Display display(ct, cli.m_sortBy, 1);

    ct.setLogFileStream(logFileStream);

    std::thread captureThread(runCapture, std::ref(pc));
    std::thread displayThread(runDisplay, std::ref(display), 1);

    captureThread.join();
    displayThread.join();

    if (logFileStream && logFileStream->is_open()) {
        logFileStream->close();
    }

    return 0;
}
