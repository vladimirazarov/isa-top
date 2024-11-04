/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#include "connectionsTable.hpp"

#define USAGE_MESSAGE "\
Usage: isa-top -i <interface> [-s <b|p>] [-l <logfile>]\n\n \
Options:\n \
-h            Display this help message and exit\n \
-i <arg>      The network interface for app to listen on\n \
-s <arg>      Sort the output by bytes or packets, <arg> is b or p accordingly\n \
-l, --log     Turn on the logging\n"

// Class to handle command line arguments
class CommandLineInterface
{
public:
    // Network interface to listen on
    std::string m_interface;
    // Sorting criteria
    SortBy m_sortBy;
    void validateRetrieveArgs();
    CommandLineInterface(int argc, char *argv[]);
    std::string m_logFilePath;

private:
    int m_argc;
    std::vector<std::string> m_argv;
};
