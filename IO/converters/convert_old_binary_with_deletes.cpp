/**
 * @file convert_old_binary_with_deletes.cpp
 *
 * Converts a binary graph file that includes inserts and deletes into the
 * extended binary format. Each row in the input is 16 bytes:
 *
 *   [uint32_t insert_flag][uint32_t src][uint32_t dst][uint32_t timestamp]
 *
 * - insert_flag: 1 means insert, 0 means delete.
 * - The graph is NOT a signal graph (i.e., rows may delete existing edges).
 *
 * Output format:
 * - 32-byte header:
 *     [uint64_t num_vertices]
 *     [uint64_t num_unique_edges]
 *     [uint64_t total_updates]
 *     [uint8_t  if_signal_graph = 0]
 *     [uint8_t  vertex_id_type = 12 (uint32_t)]
 *     [uint8_t  timestamp_type = 12 (uint32_t)]
 *     [uint8_t  if_directed = 1]
 *     [uint8_t  weight_type = 0]
 *     [padding to reach 32 bytes]
 *
 * - Followed by data rows of 13 bytes:
 *     [uint32_t src][uint32_t dst][uint32_t timestamp][uint8_t update_type]
 *       where update_type = 0 for insert, 1 for delete
 */

#include <iostream>
#include <fstream>
#include <unordered_set>
#include <set>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <tuple>
#include <algorithm>

constexpr uint8_t UINT32_TYPE = 12;
constexpr uint8_t NONE_TYPE = 0;

struct EdgePairHash {
    size_t operator()(const std::pair<uint32_t, uint32_t>& p) const {
        return std::hash<uint64_t>()(((uint64_t)p.first << 32) | p.second);
    }
};

void die(const std::string& msg) {
    std::cerr << "Error: " << msg << std::endl;
    std::exit(1);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        die("Usage: ./convert_old_binary_with_deletes <input_16b.bin> <output_new.bin>");
    }

    const char* input_filename = argv[1];
    const char* output_filename = argv[2];

    int fd = open(input_filename, O_RDONLY);
    if (fd == -1) {
        die("Failed to open input file");
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        die("Failed to stat input file");
    }

    size_t file_size = sb.st_size;
    if (file_size % 16 != 0) {
        close(fd);
        die("Input file size is not a multiple of 16 bytes");
    }

    size_t num_records = file_size / 16;
    void* mapped = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        die("Failed to mmap input file");
    }

    close(fd);

    const uint32_t* data = reinterpret_cast<const uint32_t*>(mapped);
    std::vector<std::tuple<uint32_t, uint32_t, bool, uint32_t>> data_vector;
    std::unordered_set<std::pair<uint32_t, uint32_t>, EdgePairHash> unique_edges;

    // First pass — compute stats
    uint32_t max_vertex_id = 0;
    for (size_t i = 0; i < num_records; ++i) {
        uint32_t src = data[i * 4 + 1];
        uint32_t dst = data[i * 4 + 2];
        data_vector.emplace_back(src, dst, data[i * 4 + 0], data[i * 4 + 3]);
        unique_edges.emplace(src, dst);
        max_vertex_id = std::max({max_vertex_id, src, dst});
        if (i % 100000000 == 0) {
            std::cout << i << "\n";
        }
    }

    std::sort(data_vector.begin(), data_vector.end(), [](auto const &t1, auto const &t2) {
      return std::get<3>(t1) < std::get<3>(t2);
    });

    // Create dense ID map
    std::unordered_map<uint32_t, uint32_t> dense_id_map;
    uint32_t dense_id = 0;
    for (auto & [src, dest, unused1, unused2] : data_vector) {
        if (dense_id_map.count(src) == 0) {
            dense_id_map[src] = dense_id++;
        }
        if (dense_id_map.count(dest) == 0) {
            dense_id_map[dest] = dense_id++;
        }
        
    }

    uint64_t num_vertices = dense_id_map.size();
    uint64_t num_unique_edges = unique_edges.size();
    uint64_t total_updates = num_records;

    std::cout << total_updates << " updates (with deletes), "
          << num_unique_edges << " unique edges, "
          << num_vertices << " vertices.\n"
          << "Maximum vertex ID encountered: " << max_vertex_id << std::endl;

    // Open output
    std::ofstream out(output_filename, std::ios::binary);
    if (!out) {
        munmap(mapped, file_size);
        die("Failed to open output file");
    }

    // Write header
    out.write(reinterpret_cast<char*>(&num_vertices), sizeof(uint64_t));
    out.write(reinterpret_cast<char*>(&num_unique_edges), sizeof(uint64_t));
    out.write(reinterpret_cast<char*>(&total_updates), sizeof(uint64_t));
    uint8_t signal_graph = 0;  // not a signal graph
    out.write(reinterpret_cast<char*>(&signal_graph), 1);
    out.write(reinterpret_cast<const char*>(&UINT32_TYPE), 1);  // vertex_id_type
    out.write(reinterpret_cast<const char*>(&UINT32_TYPE), 1);  // timestamp_type
    uint8_t directed = 1;
    out.write(reinterpret_cast<char*>(&directed), 1);
    out.write(reinterpret_cast<const char*>(&NONE_TYPE), 1);    // weight_type

    uint8_t padding[32 - 8 * 3 - 5] = {};
    out.write(reinterpret_cast<char*>(padding), sizeof(padding));

    // Second pass — write data
    for (size_t i = 0; i < num_records; ++i) {
        uint8_t is_delete = std::get<2>(data_vector[i]);
        uint32_t src = std::get<0>(data_vector[i]);
        uint32_t dst = std::get<1>(data_vector[i]);
        uint32_t ts  = std::get<3>(data_vector[i]);

        uint32_t new_src = dense_id_map[src];
        uint32_t new_dst = dense_id_map[dst];

        out.write(reinterpret_cast<const char*>(&new_src), sizeof(uint32_t));
        out.write(reinterpret_cast<const char*>(&new_dst), sizeof(uint32_t));
        out.write(reinterpret_cast<const char*>(&ts), sizeof(uint32_t));
        out.write(reinterpret_cast<const char*>(&is_delete), sizeof(uint8_t));
        if (i % 100000000 == 0) {
            std::cout << i << "\n";
        }
    }


    out.close();
    munmap(mapped, file_size);

    std::cout << "Wrote " << total_updates << " updates (with deletes), "
          << num_unique_edges << " unique edges, "
          << num_vertices << " vertices.\n"
          << "Maximum vertex ID encountered: " << max_vertex_id << std::endl;

    return 0;
}
