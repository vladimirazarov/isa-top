#include "cli.hpp"

CommandLineInterface::CommandLineInterface(int argc, char *argv[])
{
    m_argc = argc;
    for (int i = 0; i < argc; i++)
    {
        m_argv.push_back(argv[i]);
    }
};

void CommandLineInterface::validateRetrieveArgs() {
    if (m_argc < 3 || m_argc > 6) {
        std::cerr << USAGE_MESSAGE << std::endl;
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < m_argc; ++i) {
        if (m_argv[i] == "-i" && i + 1 < m_argc) {
            m_interface = m_argv[i + 1];
            i++;
        } else if (m_argv[i] == "-s" && i + 1 < m_argc) {
            if (m_argv[i + 1] == "b")
                m_sortBy = SortBy::BY_BYTES;
            else if (m_argv[i + 1] == "p")
                m_sortBy = SortBy::BY_PACKETS;
            else {
                std::cerr << USAGE_MESSAGE << std::endl;
                exit(EXIT_FAILURE);
            }
            i++;
        } else if (m_argv[i] == "-l" || m_argv[i] == "--log") {
            m_logFilePath = "log.csv";  
        } else {
            std::cerr << USAGE_MESSAGE << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

