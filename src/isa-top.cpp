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

ConnectionsTable* globalConnectionsTable = nullptr;

void signalHandler(int signum) {
    exit(signum);
}

#define USAGE_MESSAGE "\
Usage:  isa-top [OPTIONS] ... [ARGUMENTS] ... \
\
Options: \
    -h,         Display this help message and exit \
    -i <arg>,   The network interface for app to listen on \
    -s <arg>,   Sort the output by bytes or packets, <arg> is b or p accordingly"



void run(PacketCapture& pc, Display& display, int updateInterval) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(updateInterval));
        display.update();
        if (getch() == 'q') { // Allow quitting with 'q'
            pc.stopCapture();
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    ArgumentParser ap(argc, argv);
    ap.validateRetrieveArgs();

    ConnectionsTable ct;
    globalConnectionsTable = &ct;

    std::signal(SIGINT, signalHandler);

    PacketCapture pc(ap.m_interface, *globalConnectionsTable);
    
    Display display(*globalConnectionsTable, ap.m_sortBy, 1); 
    display.run();

    pc.startCapture();

    run(pc, display, 1); 

    return 0;
}