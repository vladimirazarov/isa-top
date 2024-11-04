CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -MMD -MP -g
LDFLAGS = -lpcap -lncurses -lgtest -lgtest_main -lpthread

TARGET = isa-top
SRCS = src/connection.cpp src/packet.cpp src/connectionID.cpp src/connectionsTable.cpp src/cli.cpp src/display.cpp src/isa-top.cpp
OBJS = $(SRCS:.cpp=.o)

TEST_SRCS = test/unit.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
TEST_TARGET = unit_tests

INT_TEST_SRCS = test/int.cpp
INT_TEST_OBJS = $(INT_TEST_SRCS:.cpp=.o)
INT_TEST_TARGET = integration_tests

TEST_DEPS = src/connection.o src/packet.o src/connectionID.o src/connectionsTable.o src/cli.o src/display.o

DEPS = $(OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(INT_TEST_OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

unit_tests: $(TEST_OBJS) $(TEST_DEPS)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) $(TEST_DEPS) $(LDFLAGS)

integration_tests: $(INT_TEST_OBJS) $(TEST_DEPS)
	$(CXX) $(CXXFLAGS) -o $(INT_TEST_TARGET) $(INT_TEST_OBJS) $(TEST_DEPS) $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test/%.o: test/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(INT_TEST_OBJS) $(DEPS) $(TARGET) $(TEST_TARGET) $(INT_TEST_TARGET)

.PHONY: all clean unit_tests integration_tests
