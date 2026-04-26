CXX := g++
CXXFLAGS := -std=c++20 -O3 -Wall -Wextra

# Osmium include path (Ubuntu/Debian default)
OSMIUM_INCLUDE := -I/usr/include/osmium

# Libraries for OSM processing (libosmium is header-only)
OSMIUM_LIBS := -lexpat -lz -lbz2 -llz4 -lprotobuf

BINARIES := osm_extractor csv_to_bin

all: $(BINARIES)

osm_extractor: openstreetmap_dynamic.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(OSMIUM_LIBS)

csv_to_bin: csv_to_bin.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OSMIUM_INCLUDE) -c $< -o $@

# Clean up
clean:
	rm -f $(BINARIES) *.o

ignore:
	rm -f *.o $(BINARIES)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt update
	sudo apt install -y libosmium2-dev libexpat1-dev zlib1g-dev libbz2-dev liblz4-dev libprotobuf-dev

.PHONY: all clean install-deps
