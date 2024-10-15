CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -MMD -MP -g
LDFLAGS = -lpcap -lncurses 

TARGET = isa-top
SRCS = src/connection.cpp src/isa-top.cpp src/packet.cpp src/connectionID.cpp src/connectionsTable.cpp src/cli.cpp src/display.cpp
OBJS = $(SRCS:.cpp=.o)

DEPS = $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS)

.PHONY: all clean
