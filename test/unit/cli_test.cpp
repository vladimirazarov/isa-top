#include "../../src/cli.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <string>

std::vector<char*> createArgv(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr); 
    return argv;
}

class StderrCapture {
public:
    StderrCapture() {
        oldStderr = std::cerr.rdbuf();
        std::cerr.rdbuf(errStream.rdbuf());
    }

    ~StderrCapture() {
        std::cerr.rdbuf(oldStderr);
    }

    std::string getCapturedStderr() const {
        return errStream.str();
    }

private:
    std::streambuf* oldStderr;
    std::stringstream errStream;
};


TEST(CommandLineInterfaceTest, ValidInterfaceOnly) {
    std::vector<std::string> args = {"program", "-i", "eth0"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    EXPECT_NO_THROW({
        CommandLineInterface cli(argc, argv.data());
        cli.validateRetrieveArgs();
    });
}

TEST(CommandLineInterfaceTest, ValidInterfaceAndSort) {
    std::vector<std::string> args = {"program", "-i", "eth0", "-s", "p"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    EXPECT_NO_THROW({
        CommandLineInterface cli(argc, argv.data());
        cli.validateRetrieveArgs();
    });
}

TEST(CommandLineInterfaceTest, MissingInterface) {
    std::vector<std::string> args = {"program", "-s", "b"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    EXPECT_EXIT({
        CommandLineInterface cli(argc, argv.data());
        cli.validateRetrieveArgs();
    }, ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST(CommandLineInterfaceTest, InvalidSortOption) {
    std::vector<std::string> args = {"program", "-i", "eth0", "-s", "x"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    EXPECT_EXIT({
        CommandLineInterface cli(argc, argv.data());
        cli.validateRetrieveArgs();
    }, ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST(CommandLineInterfaceTest, UnknownOption) {
    std::vector<std::string> args = {"program", "-i", "eth0", "-x", "foo"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    EXPECT_EXIT({
        CommandLineInterface cli(argc, argv.data());
        cli.validateRetrieveArgs();
    }, ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST(CommandLineInterfaceTest, DuplicateInterfaceOption) {
    std::vector<std::string> args = {"program", "-i", "eth0", "-i", "eth1"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    EXPECT_NO_THROW({
        CommandLineInterface cli(argc, argv.data());
        cli.validateRetrieveArgs();
    });
}

TEST(CommandLineInterfaceTest, OptionsInDifferentOrder) {
    std::vector<std::string> args = {"program", "-s", "b", "-i", "eth0"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    EXPECT_NO_THROW({
        CommandLineInterface cli(argc, argv.data());
        cli.validateRetrieveArgs();
    });
}

TEST(CommandLineInterfaceTest, ExtraArguments) {
    std::vector<std::string> args = {"program", "-i", "eth0", "extra_arg"};
    std::vector<char*> argv = createArgv(args);
    int argc = args.size();

    EXPECT_EXIT({
        CommandLineInterface cli(argc, argv.data());
        cli.validateRetrieveArgs();
    }, ::testing::ExitedWithCode(EXIT_FAILURE), "");
}
