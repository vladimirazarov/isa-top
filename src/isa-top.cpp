#include <iostream>
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
    if (globalConnectionsTable) {
        globalConnectionsTable->printConnections();
    }
    exit(signum);
}

#define USAGE_MESSAGE "\
Usage:  isa-top [OPTIONS] ... [ARGUMENTS] ... \
\
Options: \
    -h,         Display this help message and exit \
    -i <arg>,   The network interface for app to listen on \
    -s <arg>,   Sort the output by bytes or packets, <arg> is b or p accordingly"

class ArgumentParser {
public:

    std::string m_interface;
    std::string m_sortBy;

    ArgumentParser(int argc, char *argv[]){
        m_argc = argc;
        for (int i = 0; i<argc; i++){
            m_argv.push_back(argv[i]);
        }
    };

    // Method to validate and retrieve arguments
    void validateRetrieveArgs() {
        if (m_argc > 5 || m_argc < 3){
            std::cerr << USAGE_MESSAGE << std::endl;
            exit(EXIT_FAILURE);
        }

        for (int i = 1; i < m_argc; ++i) {
            if (m_argv[i] == "-i" && i + 1 < m_argc) {
                m_interface = m_argv[i + 1];
                i++; 
            }
            else if (m_argv[i] == "-s" && i + 1 < m_argc) {
                m_sortBy = m_argv[i + 1];
                i++; 
            }
            else {
                std::cerr << USAGE_MESSAGE << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }

private:
    int m_argc;
    std::vector<std::string> m_argv;
};

int main(int argc, char *argv[]) {
    ArgumentParser ap(argc, argv);
    ap.validateRetrieveArgs();

    ConnectionsTable ct;
    globalConnectionsTable = &ct;

    std::signal(SIGINT, signalHandler);

    PacketCapture pc(ap.m_interface, *globalConnectionsTable);

    pc.startCapture();

    return 0;
}