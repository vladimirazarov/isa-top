#include "../../src/cli.hpp"
#include "../../src/packet.hpp"
#include "../../src/connectionsTable.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <string>

static std::vector<char*> createArgv(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr); 
    return argv;
}

TEST(CommandLineInterfaceIntegrationTest, CLIInitializesPacketCapture) {
    std::vector<std::string> args = {"program", "-i", "eth0", "-s", "b"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    CommandLineInterface cli(argc, argv.data());
    cli.validateRetrieveArgs();

    ConnectionsTable connectionsTable;

    PacketCapture packetCapture(cli.m_interface, connectionsTable);

    EXPECT_EQ(packetCapture.m_interfaceName, "eth0");

}
