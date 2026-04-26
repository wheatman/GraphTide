CXX := g++
CXXFLAGS := -std=c++20 -O3 -Wall -Wextra -march=native -I../../external/parlaylib/include/

TARGET := osm4routing2pbbs

SRCS := osm4routing2pbbs.cpp

OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(TARGET) $(OBJS)

