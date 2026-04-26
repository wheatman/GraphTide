/**
 * @file convert_old_binary_signal.cpp
 *
 * This tool converts a binary graph file from an **old raw format** into a 
 * **structured binary format** with a header.
 *
 * -------------------------
 * INPUT FORMAT (old format)
 * -------------------------
 * - Raw binary file with no header.
 * - Each row is exactly 12 bytes:
 *     [uint32_t source][uint32_t destination][uint32_t timestamp]
 * - All values are 32-bit unsigned integers.
 * - No vertex remapping (IDs are assumed to be arbitrary, possibly sparse).
 *
 * --------------------------
 * OUTPUT FORMAT (new format)
 * --------------------------
 * - Binary file begins with a 32-byte header:
 *     * [uint64_t num_vertices]
 *     * [uint64_t num_unique_edges]
 *     * [uint64_t total_updates]
 *     * [uint8_t  if_signal_graph = 1]
 *     * [uint8_t  vertex_id_type = UINT32]
 *     * [uint8_t  timestamp_type = UINT32]
 *     * [uint8_t  if_directed = 1]
 *     * [uint8_t  weight_type = NONE]
 *     * [padding  (11 bytes) to reach 32 total bytes]
 * - Followed by raw binary data:
 *     * Each edge is stored as three 32-bit unsigned integers: 
 *         [src][dst][timestamp]
 *
 * --------------------------
 * USAGE
 * --------------------------
 *   ./convert_old_binary_to_new input_old.bin output_new_format.bin
 *
 * The program uses `mmap` to read the input file efficiently and scans
 * it to compute the number of unique vertices and edges before writing.
 */

#include <iostream>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>
#include <set>

constexpr uint8_t UINT32_TYPE = 12;
constexpr uint8_t NONE_TYPE = 0;

struct EdgePairHash {
    size_t operator()(const std::pair<uint32_t, uint32_t>& p) const {
        return std::hash<uint64_t>()((uint64_t)p.first << 32 | p.second);
    }
};

void die(const std::string& msg) {
    std::cerr << msg << std::endl;
    std::exit(1);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        die("Usage: ./convert_old_binary_signal <input.txt> <output.bin>");
    }

    const char* input_filename = argv[1];
    const char* output_filename = argv[2];

    std::ifstream ifs(input_filename);

    std::unordered_map<std::string, unsigned> vertices_mapping;
    size_t n = 0, m = 0;

    std::string u, v;
    unsigned year;
    std::vector<uint32_t> data;
    while (ifs >> u >> v) {
      ifs >> year;
      m++;
      if (!vertices_mapping.contains(u)) {
        vertices_mapping[u] = n++;
      }
      if (!vertices_mapping.contains(v)) {
        vertices_mapping[v] = n++;
      }
      if (m < 5) {
        printf("edge(%s,%s,%d) -> (%u,%u,%u)\n", u.c_str(), v.c_str(), year, vertices_mapping[u], vertices_mapping[v], year);
      }
      data.push_back(vertices_mapping[u]);
      data.push_back(vertices_mapping[v]);
      data.push_back(year);
    }

    std::unordered_set<uint32_t> vertices;
    std::unordered_set<std::pair<uint32_t, uint32_t>, EdgePairHash> unique_edges;

    auto num_edges = m;
    for (size_t i = 0; i < num_edges; ++i) {
        uint32_t src = data[i * 3 + 0];
        uint32_t dst = data[i * 3 + 1];
        vertices.insert(src);
        vertices.insert(dst);
        unique_edges.emplace(src, dst);
        if (i % 100000000 == 0) {
            std::cout << i << "\n";
        }
    }

    uint64_t num_vertices = vertices.size();
    uint64_t num_unique_edges = unique_edges.size();
    uint64_t total_updates = num_edges;

    std::cout << total_updates << " updates, "
              << num_unique_edges << " unique edges, "
              << num_vertices << " vertices." << std::endl;

    std::ofstream out(output_filename, std::ios::binary);
    if (!out) {
        die("Failed to open output file for writing");
    }

    // Header
    out.write(reinterpret_cast<char*>(&num_vertices), sizeof(uint64_t));
    out.write(reinterpret_cast<char*>(&num_unique_edges), sizeof(uint64_t));
    out.write(reinterpret_cast<char*>(&total_updates), sizeof(uint64_t));
    uint8_t signal_graph = 1;
    out.write(reinterpret_cast<char*>(&signal_graph), 1);
    out.write(reinterpret_cast<const char*>(&UINT32_TYPE), 1);
    out.write(reinterpret_cast<const char*>(&UINT32_TYPE), 1);
    uint8_t directed = 1;
    out.write(reinterpret_cast<char*>(&directed), 1);
    out.write(reinterpret_cast<const char*>(&NONE_TYPE), 1);

    uint8_t padding[32 - 8 * 3 - 5] = {};
    out.write(reinterpret_cast<char*>(padding), sizeof(padding));

    for (size_t i = 0; i < num_edges; ++i) {
        out.write(reinterpret_cast<const char*>(&data[i * 3 + 0]), sizeof(uint32_t));
        out.write(reinterpret_cast<const char*>(&data[i * 3 + 1]), sizeof(uint32_t));
        out.write(reinterpret_cast<const char*>(&data[i * 3 + 2]), sizeof(uint32_t));
        if (i % 100000000 == 0) {
            std::cout << i << "\n";
        }
    }

    out.close();

    std::cout << "Wrote " << total_updates << " updates, "
              << num_unique_edges << " unique edges, "
              << num_vertices << " vertices." << std::endl;

    return 0;
}
