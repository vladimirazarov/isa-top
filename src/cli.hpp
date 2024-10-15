#include "connectionsTable.hpp"

#define USAGE_MESSAGE "\
Usage:  isa-top [OPTIONS] ... [ARGUMENTS] ... \n\
\n\
Options:\n\
    -h          Display this help message and exit \n \
    -i <arg>,   The network interface for app to listen on \n \
    -s <arg>,   Sort the output by bytes or packets, <arg> is b or p accordingly \n"

class CommandLineInterface{ 
public:
    std::string m_interface;
    SortBy m_sortBy;
    void validateRetrieveArgs();
    CommandLineInterface(int argc, char *argv[]);
private:
    int m_argc;
    std::vector<std::string> m_argv;
};

