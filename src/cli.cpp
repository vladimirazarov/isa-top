#include "cli.hpp"

class CommandLineInterface{ 
public:
    std::string m_interface;
    std::string m_sortBy;

    CommandLineInterface(int argc, char *argv[]){
        m_argc = argc;
        for (int i = 0; i<argc; i++){
            m_argv.push_back(argv[i]);
        }
    };

    //TODO: convert string (b, p) to the type SortBy and validate that right value is passed (ob or p) 
    // Method to validat and retrieve arguments
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
