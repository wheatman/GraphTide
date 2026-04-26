#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_map>
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

struct PairHash {
    size_t operator()(const std::pair<uint64_t, uint64_t>& p) const {
        size_t h1 = std::hash<uint64_t>{}(p.first);
        size_t h2 = std::hash<uint64_t>{}(p.second);
        return h1 ^ (h2 * 0x9e3779b97f4a7c15ULL + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

struct GraphStats {
    std::string name;
    uint64_t num_vertices;
    uint64_t num_edges;
    uint64_t total_updates;
    // Bipartite check: no vertex appears as both src and dst
    bool is_bipartite;
    // Star check: one vertex connects to all others
    bool is_star;
    uint64_t star_center;
    uint64_t star_degree;
    // Degree stats
    uint64_t max_out_degree;
    uint64_t max_out_vertex;
    uint64_t max_in_degree;
    uint64_t max_in_vertex;
    uint64_t unique_src_count;
    uint64_t unique_dst_count;
    uint64_t overlap_count;  // vertices appearing as both src and dst
    // Parallel edge stats
    bool has_parallel_edges;
    uint64_t unique_edge_pairs;     // distinct (src, dst) pairs
    uint64_t parallel_edge_pairs;   // (src, dst) pairs with >1 different timestamp
    uint64_t max_parallel_count;    // max number of events for a single (src, dst)
    uint64_t max_parallel_src;
    uint64_t max_parallel_dst;
};

GraphStats analyze_graph(const std::string& path) {
    GraphStats stats{};

    // Extract dataset name
    auto slash = path.rfind('/');
    auto dot = path.rfind('.');
    stats.name = path.substr(slash + 1, dot - slash - 1);

    std::ifstream fin(path, std::ios::binary);
    if (!fin) {
        fprintf(stderr, "Cannot open %s\n", path.c_str());
        return stats;
    }

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

    stats.num_vertices = h.num_vertices;
    stats.num_edges = h.num_unique_edges;
    stats.total_updates = h.total_updates;

    size_t vid_sz = type_size(h.vid_type);
    size_t ts_sz = type_size(h.ts_type);
    size_t wt_sz = type_size(h.wt_type);
    size_t info_sz = h.signal ? 0 : 1;
    size_t row_sz = 2 * vid_sz + ts_sz + wt_sz + info_sz;

    // For bipartite: track which vertices appear as src vs dst
    std::unordered_set<uint64_t> src_set, dst_set;

    // For degree distribution
    std::unordered_map<uint64_t, uint64_t> out_degree, in_degree;

    // For parallel edge detection: count events per (src, dst) pair
    std::unordered_map<std::pair<uint64_t, uint64_t>, uint64_t, PairHash> edge_count;

    const size_t BUF_ROWS = 1 << 20;  // ~1M rows per chunk
    std::vector<char> buf(BUF_ROWS * row_sz);

    uint64_t rows_processed = 0;
    while (fin) {
        fin.read(buf.data(), BUF_ROWS * row_sz);
        size_t bytes_read = fin.gcount();
        size_t rows = bytes_read / row_sz;

        const char* ptr = buf.data();
        for (size_t i = 0; i < rows; i++, ptr += row_sz) {
            uint64_t src = 0, dst = 0;
            memcpy(&src, ptr, vid_sz);
            memcpy(&dst, ptr + vid_sz, vid_sz);

            // For update graphs, only count inserts (info byte == 1 for insert)
            if (!h.signal) {
                uint8_t info = 0;
                memcpy(&info, ptr + 2 * vid_sz + ts_sz + wt_sz, 1);
                if (info != 1) continue;  // skip deletes
            }

            src_set.insert(src);
            dst_set.insert(dst);
            out_degree[src]++;
            in_degree[dst]++;
            edge_count[{src, dst}]++;
        }
        rows_processed += rows;

        if (rows_processed % (100ULL << 20) == 0 && rows_processed > 0) {
            fprintf(stderr, "  %s: processed %lu / %lu rows...\n",
                    stats.name.c_str(), rows_processed, h.total_updates);
        }
    }

    // Bipartite check
    stats.unique_src_count = src_set.size();
    stats.unique_dst_count = dst_set.size();
    uint64_t overlap = 0;
    for (auto& v : src_set) {
        if (dst_set.count(v)) overlap++;
    }
    stats.overlap_count = overlap;
    stats.is_bipartite = (overlap == 0);

    // Max degrees
    stats.max_out_degree = 0;
    stats.max_out_vertex = 0;
    for (auto& [v, deg] : out_degree) {
        if (deg > stats.max_out_degree) {
            stats.max_out_degree = deg;
            stats.max_out_vertex = v;
        }
    }
    stats.max_in_degree = 0;
    stats.max_in_vertex = 0;
    for (auto& [v, deg] : in_degree) {
        if (deg > stats.max_in_degree) {
            stats.max_in_degree = deg;
            stats.max_in_vertex = v;
        }
    }

    // Star check: a star graph has one center vertex connected to all others
    // Check if max degree == total_unique_vertices - 1
    uint64_t total_unique_verts = 0;
    {
        std::unordered_set<uint64_t> all_verts(src_set.begin(), src_set.end());
        all_verts.insert(dst_set.begin(), dst_set.end());
        total_unique_verts = all_verts.size();
    }

    // Check out-degree star (one src connects to all others)
    // or in-degree star (one dst receives from all others)
    uint64_t max_total_degree = 0;
    uint64_t max_total_vertex = 0;

    // Merge in+out degrees
    std::unordered_map<uint64_t, uint64_t> total_degree;
    for (auto& [v, d] : out_degree) total_degree[v] += d;
    for (auto& [v, d] : in_degree) total_degree[v] += d;

    for (auto& [v, d] : total_degree) {
        if (d > max_total_degree) {
            max_total_degree = d;
            max_total_vertex = v;
        }
    }

    // Star if one vertex has degree == all other vertices
    stats.is_star = (total_unique_verts > 1 &&
                     max_total_degree == total_unique_verts - 1);
    stats.star_center = max_total_vertex;
    stats.star_degree = max_total_degree;

    // Parallel edge stats
    stats.unique_edge_pairs = edge_count.size();
    stats.parallel_edge_pairs = 0;
    stats.max_parallel_count = 0;
    stats.max_parallel_src = 0;
    stats.max_parallel_dst = 0;
    for (auto& [edge, cnt] : edge_count) {
        if (cnt > 1) {
            stats.parallel_edge_pairs++;
        }
        if (cnt > stats.max_parallel_count) {
            stats.max_parallel_count = cnt;
            stats.max_parallel_src = edge.first;
            stats.max_parallel_dst = edge.second;
        }
    }
    stats.has_parallel_edges = (stats.parallel_edge_pairs > 0);

    return stats;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1.bin> [file2.bin ...]\n", argv[0]);
        return 1;
    }

    printf("%-25s %15s %15s %15s %12s %12s %12s %10s %10s %10s\n",
           "Dataset", "|V|", "|E|", "Updates",
           "#Src", "#Dst", "#Overlap", "Bipart?", "Star?", "Multi?");
    printf("%-25s %15s %15s %15s %12s %12s %12s %10s %10s %10s\n",
           "-------", "---", "---", "-------",
           "----", "----", "--------", "-------", "-----", "------");

    for (int i = 1; i < argc; i++) {
        fprintf(stderr, "Analyzing %s ...\n", argv[i]);
        auto s = analyze_graph(argv[i]);

        printf("%-25s %15lu %15lu %15lu %12lu %12lu %12lu %10s %10s %10s\n",
               s.name.c_str(), s.num_vertices, s.num_edges, s.total_updates,
               s.unique_src_count, s.unique_dst_count, s.overlap_count,
               s.is_bipartite ? "YES" : "NO",
               s.is_star ? "YES" : "NO",
               s.has_parallel_edges ? "YES" : "NO");

        printf("  Max out-degree: %lu (vertex %lu)\n", s.max_out_degree, s.max_out_vertex);
        printf("  Max in-degree:  %lu (vertex %lu)\n", s.max_in_degree, s.max_in_vertex);
        printf("  Star center:    vertex %lu (total degree %lu / %lu vertices)\n",
               s.star_center, s.star_degree, s.num_vertices);
        printf("  Unique (src,dst) pairs: %lu, with parallel edges: %lu (%.2f%%)\n",
               s.unique_edge_pairs, s.parallel_edge_pairs,
               s.unique_edge_pairs > 0 ? 100.0 * s.parallel_edge_pairs / s.unique_edge_pairs : 0.0);
        printf("  Max parallel count: %lu (edge %lu -> %lu)\n",
               s.max_parallel_count, s.max_parallel_src, s.max_parallel_dst);
        printf("\n");
    }

    return 0;
}
