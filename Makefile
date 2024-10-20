CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -MMD -MP -g
LDFLAGS = -lpcap -lncurses -lgtest -lgtest_main -lpthread

TARGET = isa-top
SRCS = src/connection.cpp src/packet.cpp src/connectionID.cpp src/connectionsTable.cpp src/cli.cpp src/display.cpp
OBJS = $(SRCS:.cpp=.o)

# Unit tests
TEST_SRCS = test/unit/connections_table_test.cpp test/unit/connectionID_test.cpp test/unit/cli_test.cpp test/unit/packet_capture_test.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
TEST_TARGET = unit_tests

# Integration tests
INT_TEST_SRCS = \
    test/int/packet_capture_integration_test.cpp \
    test/int/cli_packet_capture_integration_test.cpp \
    test/int/packet_capture_display_integration_test.cpp \
    test/int/full_system_integration_test.cpp

INT_TEST_OBJS = $(INT_TEST_SRCS:.cpp=.o)
INT_TEST_TARGET = integration_tests

DEPS = $(OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(INT_TEST_OBJS:.o=.d)

all: $(TARGET) 

$(TARGET): $(OBJS) src/isa-top.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) src/isa-top.o $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) $(OBJS) $(LDFLAGS)

$(INT_TEST_TARGET): $(INT_TEST_OBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(INT_TEST_TARGET) $(INT_TEST_OBJS) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(INT_TEST_OBJS) src/isa-top.o $(DEPS) $(TEST_TARGET) $(INT_TEST_TARGET)

.PHONY: all clean
