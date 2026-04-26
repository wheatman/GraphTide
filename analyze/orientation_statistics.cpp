#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "graph.hpp"
#include "graph_generators.hpp"
#include "orientation.hpp"
#include "io_util.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include "../IO/IO.hpp"

#ifndef NODE_TYPE
#define NODE_TYPE uint32_t
#endif

#ifndef TIMESTAMP_TYPE
#define TIMESTAMP_TYPE uint32_t
#endif


template <class E>
void orientation_statistics(size_t n, E& edges, size_t print_freq, std::string& output_dir) {
    std::filesystem::create_directories(output_dir);
    std::ofstream stats_file(std::filesystem::path(output_dir) / "statistics.csv");
    stats_file << "timestep, total flips, max out-degree" << std::endl;

    size_t k = log2(n);
    EdgeOrientation orientation(n, k);
    size_t total_flips = 0;

    std::cout << "Running Incremental Orientation Algorithm." << std::endl;
    std::cout << "    n=" << n << ", k=" << k << std::endl;
    for (size_t i = 0; i < edges.size(); i++) {
        total_flips += orientation.insert_edge(std::get<0>(edges[i]), std::get<1>(edges[i]));
        if (i % print_freq == 0) {
            size_t max_deg = orientation.get_max_out_degree();
            std::cout << "Processed " << i << " out of " << edges.size() << " edges. " << std::endl;
            std::cout << "Maximum Out-Degree: " << max_deg << std::endl;
            std::cout << "Total Edge Flips: " << total_flips << std::endl;
            std::cout << std::endl;
            stats_file << i << ", " << total_flips << ", " << max_deg << std::endl;
            stats_file.close();
        }
    }
}

ABSL_FLAG(std::string, output, "orientation_results", "output directory");
ABSL_FLAG(std::string, input, "", "input filename");
ABSL_FLAG(bool, edges_shuffle, false, "Shuffle the order for edges");
ABSL_FLAG(int64_t, max_updates, -1, "max updates to read");
ABSL_FLAG(size_t, print_freq, 1000000, "How often to dump stats to file");

int main(int32_t argc, char *argv[]) {
    absl::ParseCommandLine(argc, argv);
    std::string output_filedir = absl::GetFlag(FLAGS_output);
    IO::GraphHeader header(absl::GetFlag(FLAGS_input));
    size_t print_freq = absl::GetFlag(FLAGS_print_freq);

    if (header.if_signal_graph) {
        std::cout << "loading the graph" << std::endl;
        auto [edges, num_nodes] = IO::get_unweighted_graph_edges_signal<NODE_TYPE, TIMESTAMP_TYPE>(
            absl::GetFlag(FLAGS_input),
            absl::GetFlag(FLAGS_edges_shuffle),
            absl::GetFlag(FLAGS_max_updates)
        );
        std::cout << "done loading the graph" << std::endl;
        std::cout << "have " << edges.size() << " edges and " << num_nodes << " nodes" << std::endl;
        if (num_nodes > 0) {
        std::cout << "output file directory is " << output_filedir << std::endl;
        std::cout << "shuffling the edges: " << absl::GetFlag(FLAGS_edges_shuffle) << std::endl;
        orientation_statistics(num_nodes, edges, print_freq, output_filedir);
        }
    } else {
        std::cout << "NOT SUPPORTED YET." << std::endl;
        return 1;
    }

    return 0;
}
