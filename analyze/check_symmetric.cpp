// Check if a graph is symmetric: for every (src,dst), does (dst,src) also exist?
// Reports:
//   - unique directed (src,dst) pairs
//   - how many have a matching reverse pair
//   - how many (unordered) pairs are fully symmetric vs one-directional
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

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

static size_t type_size(uint8_t t) {
    switch (t) {
        case 10: return 1; case 11: return 2; case 12: return 4; case 13: return 8;
        case 20: return 1; case 21: return 2; case 22: return 4; case 23: return 8;
        case 30: return 4; case 31: return 8;
        default: return 0;
    }
}

struct PairHash {
    size_t operator()(const std::pair<uint64_t, uint64_t>& p) const {
        size_t h1 = std::hash<uint64_t>{}(p.first);
        size_t h2 = std::hash<uint64_t>{}(p.second);
        return h1 ^ (h2 * 0x9e3779b97f4a7c15ULL + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1.bin> [file2.bin ...]\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        const char* path = argv[i];
        fprintf(stderr, "=== %s ===\n", path);

        std::ifstream fin(path, std::ios::binary);
        if (!fin) { fprintf(stderr, "Cannot open\n"); continue; }

        uint8_t raw[32];
        fin.read(reinterpret_cast<char*>(raw), 32);

        Header h;
        memcpy(&h.num_vertices, raw, 8);
        memcpy(&h.num_unique_edges, raw + 8, 8);
        memcpy(&h.total_updates, raw + 16, 8);
        h.signal = raw[24]; h.vid_type = raw[25]; h.ts_type = raw[26];
        h.directed = raw[27]; h.wt_type = raw[28];

        size_t vid_sz = type_size(h.vid_type);
        size_t ts_sz = type_size(h.ts_type);
        size_t wt_sz = type_size(h.wt_type);
        size_t info_sz = h.signal ? 0 : 1;
        size_t row_sz = 2 * vid_sz + ts_sz + wt_sz + info_sz;

        // Pass 1: collect all unique (src, dst) pairs
        std::unordered_set<std::pair<uint64_t, uint64_t>, PairHash> pairs;
        pairs.reserve(h.num_unique_edges);

        const size_t BUF_ROWS = 1 << 20;
        std::vector<char> buf(BUF_ROWS * row_sz);

        uint64_t rows_processed = 0;
        while (fin) {
            fin.read(buf.data(), BUF_ROWS * row_sz);
            size_t bytes_read = fin.gcount();
            size_t rows = bytes_read / row_sz;

            const char* ptr = buf.data();
            for (size_t k = 0; k < rows; k++, ptr += row_sz) {
                uint64_t src = 0, dst = 0;
                memcpy(&src, ptr, vid_sz);
                memcpy(&dst, ptr + vid_sz, vid_sz);

                if (!h.signal) {
                    uint8_t info = 0;
                    memcpy(&info, ptr + 2 * vid_sz + ts_sz + wt_sz, 1);
                    if (info != 1) continue;
                }

                pairs.insert({src, dst});
            }
            rows_processed += rows;
            if (rows_processed % (100ULL << 20) == 0 && rows_processed > 0) {
                fprintf(stderr, "  pass1: %lu rows, %lu unique pairs so far\n",
                        rows_processed, pairs.size());
            }
        }

        fprintf(stderr, "  unique (src,dst) pairs: %lu\n", pairs.size());

        // Pass 2: for each pair (a,b) with a<b, check if (a,b) and (b,a) both exist
        uint64_t symmetric_both = 0;      // both (a,b) and (b,a) exist
        uint64_t one_way = 0;             // only one direction exists
        uint64_t self_loops = 0;
        for (auto& p : pairs) {
            if (p.first == p.second) { self_loops++; continue; }
            if (p.first < p.second) {
                if (pairs.count({p.second, p.first})) symmetric_both++;
                else one_way++;
            } else {
                // counted when we see the flipped version with first<second,
                // or here if the reverse doesn't exist
                if (!pairs.count({p.second, p.first})) one_way++;
            }
        }

        uint64_t total_directed = pairs.size();
        uint64_t total_undirected = symmetric_both + one_way + self_loops;
        double symmetry_pct = total_undirected > 0
            ? 100.0 * symmetric_both / (symmetric_both + one_way) : 0.0;

        printf("%s:\n", path);
        printf("  Unique directed (src,dst) pairs: %lu\n", total_directed);
        printf("  Self-loops: %lu\n", self_loops);
        printf("  Undirected pairs with BOTH directions: %lu\n", symmetric_both);
        printf("  Undirected pairs with only ONE direction: %lu\n", one_way);
        printf("  Symmetry (%% of undirected pairs with both dirs): %.4f%%\n",
               symmetry_pct);
        printf("  Fully symmetric? %s\n", one_way == 0 ? "YES" : "NO");
        printf("\n");
    }
    return 0;
}
