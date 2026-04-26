
CXX = g++
CXXFLAGS = -Wall -std=c++20 -O3 -pthread

CXXFLAGS += -I.

SRCS = parse.cpp
TARGET = parse

all: $(TARGET)

$(TARGET): $(SRCS)
		$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
		rm -f $(TARGET)
