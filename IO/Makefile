CXXFLAGS := -std=c++20 -Wall -Wextra -O3 -g  -gdwarf-4

TARGET := IO_test
SRC := IO_test.cpp
HEADERS := IO.hpp

.PHONY: all clean


$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)