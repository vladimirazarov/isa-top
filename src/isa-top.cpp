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

// Global pointer to connections table
ConnectionsTable *globalConnectionsTable = nullptr;

// Clean up after Ctrl+C interrupt
void signalHandler(int signum)
{
    endwin();
    std::cerr << "Interrupt signal (" << signum << ") received. Exiting..." << std::endl;
    exit(signum);
}

// Run packet capture in its own thread
void runCapture(PacketCapture &pc)
{
    pc.startCapture();
}

// Run display in its own thread
void runDisplay(Display &display)
{
    display.run();
}

int main(int argc, char *argv[])
{
    // Get command line arguments
    CommandLineInterface cli(argc, argv);
    cli.validateRetrieveArgs();
    // Create ConnectionsTable object
    ConnectionsTable ct;
    globalConnectionsTable = &ct;

    // If --log was specified, set the log file path
    if (!cli.m_logFilePath.empty())
    {
        ct.setLogFilePath(cli.m_logFilePath);
    }
    // Signal handler for Ctrl+C
    std::signal(SIGINT, signalHandler);
    // Create PacketCapture object based on the specified interface
    PacketCapture pc(cli.m_interface, ct);
    // Create display object based on the specified sorting criteria
    Display display(ct, cli.m_sortBy, 1);

    // If --log was specified, set the log file stream
    if (!cli.m_logFilePath.empty())
    {
        ct.setLogFileStream();
    }

    // Start capture packets thread
    std::thread captureThread(runCapture, std::ref(pc));
    // Start display thread
    std::thread displayThread(runDisplay, std::ref(display));

    // Wait for both threads to finish
    captureThread.join();
    displayThread.join();

    return 0;
}