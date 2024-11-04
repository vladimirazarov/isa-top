/*
 * Author: Vladimir Azarov
 * Login:  xazaro00
 */

#include "cli.hpp"

// Constructor: Initializes argc and argv
CommandLineInterface::CommandLineInterface(int argc, char *argv[])
{
    m_argc = argc;
    for (int i = 0; i < argc; i++)
    {
        m_argv.push_back(argv[i]);
    }
};

// Method to validate and retrieve command line arguments
void CommandLineInterface::validateRetrieveArgs()
{
    bool interfaceSpecified = false;

    // Check for valid number of arguments
    if (m_argc < 2 || m_argc > 6)
    {
        std::cerr << USAGE_MESSAGE << std::endl;
        exit(EXIT_FAILURE);
    }
    // Parse each argument
    for (int i = 1; i < m_argc; ++i)
    {
        std::string arg = m_argv[i];

        if (arg == "-i" && i + 1 < m_argc)
        {
            m_interface = m_argv[++i];
            interfaceSpecified = true;
        }
        else if (arg == "-s" && i + 1 < m_argc)
        {
            std::string sortArg = m_argv[++i];
            if (sortArg == "b")
                m_sortBy = SortBy::BY_BYTES;
            else if (sortArg == "p")
                m_sortBy = SortBy::BY_PACKETS;
            else
            {
                std::cerr << USAGE_MESSAGE << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (arg == "-l" || arg == "--log")
        {
            m_logFilePath = "log.csv";
        }
        else if (arg == "-h")
        {
            std::cout << USAGE_MESSAGE << std::endl;
            exit(EXIT_SUCCESS);
        }
        else
        {
            std::cerr << USAGE_MESSAGE << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    // Ensure the interface was specified
    if (!interfaceSpecified)
    {
        std::cerr << USAGE_MESSAGE << std::endl;
        exit(EXIT_FAILURE);
    }
}