#include "connectionsTable.hpp"

#define USAGE_MESSAGE "\
Usage: isa-top -i <interface> [-s <b|p>] [-l <logfile>]\n\n \
    Options:\n \
        -h            Display this help message and exit\n \
        -i <arg>      The network interface for app to listen on\n \
        -s <arg>      Sort the output by bytes or packets, <arg> is b or p accordingly\nS \
        -l, --log     Specify the log file path\n"

class CommandLineInterface{ 
public:
    std::string m_interface;
    SortBy m_sortBy;
    void validateRetrieveArgs();
    CommandLineInterface(int argc, char *argv[]);
    std::string m_logFilePath;
private:
    int m_argc;
    std::vector<std::string> m_argv;
};

