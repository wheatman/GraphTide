#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

struct Header {
    uint64_t num_vertices;
    uint64_t num_unique_edges;
    uint64_t total_updates;
    uint8_t signal;
    uint8_t vid_type;
    uint8_t ts_type;
    uint8_t directed;
    uint8_t wt_type;
};

size_t type_size(uint8_t t) {
    switch (t) {
        case 10: return 1;
        case 11: return 2;
        case 12: return 4;
        case 13: return 8;
        case 20: return 1;
        case 21: return 2;
        case 22: return 4;
        case 23: return 8;
        case 30: return 4;
        case 31: return 8;
        default: return 0;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <binary_graphs_dir>\n", argv[0]);
        return 1;
    }
    std::string dir = argv[1];

    std::vector<fs::path> files;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".bin")
            files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());

    printf("%-40s %15s %15s %15s\n", "Dataset", "Total Updates", "Unique TS", "TS Type");
    printf("%-40s %15s %15s %15s\n", "-------", "-------------", "---------", "-------");

    for (auto& path : files) {
        std::ifstream fin(path, std::ios::binary);
        if (!fin) { fprintf(stderr, "Cannot open %s\n", path.c_str()); continue; }

        uint8_t raw[32];
        fin.read(reinterpret_cast<char*>(raw), 32);

        Header h;
        memcpy(&h.num_vertices, raw, 8);
        memcpy(&h.num_unique_edges, raw + 8, 8);
        memcpy(&h.total_updates, raw + 16, 8);
        h.signal = raw[24];
        h.vid_type = raw[25];
        h.ts_type = raw[26];
        h.directed = raw[27];
        h.wt_type = raw[28];

        size_t vid_sz = type_size(h.vid_type);
        size_t ts_sz = type_size(h.ts_type);
        size_t wt_sz = type_size(h.wt_type);
        size_t info_sz = h.signal ? 0 : 1;
        size_t row_sz = 2 * vid_sz + ts_sz + wt_sz + info_sz;

        std::string dataset = path.stem().string();

        if (ts_sz == 0) {
            printf("%-40s %15lu %15s %15s\n", dataset.c_str(), h.total_updates, "N/A", "NONE");
            continue;
        }

        size_t ts_offset = 2 * vid_sz;

        // Read all data at once
        size_t data_size = row_sz * h.total_updates;
        std::vector<char> buf(data_size);
        fin.read(buf.data(), data_size);
        size_t rows_read = fin.gcount() / row_sz;

        std::unordered_set<uint64_t> unique_ts;
        unique_ts.reserve(rows_read / 10);

        const char* ptr = buf.data();
        for (size_t i = 0; i < rows_read; i++, ptr += row_sz) {
            uint64_t ts = 0;
            memcpy(&ts, ptr + ts_offset, ts_sz);
            unique_ts.insert(ts);
        }

        const char* ts_name = (ts_sz == 4) ? "UINT32" : "UINT64";
        printf("%-40s %15lu %15zu %15s\n", dataset.c_str(), h.total_updates, unique_ts.size(), ts_name);
    }
    return 0;
}
