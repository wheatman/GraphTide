#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"

#include "../IO/IO.hpp"
#include "graph.hpp"
#include "graph_generators.hpp"
#include "helpers.hpp"
#include "io_util.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "append_algorithms/bfs.hpp"
#include "append_algorithms/cc.hpp"
#include "append_algorithms/tc.hpp"

#include "dynamic_algorithms/cc.hpp"
#include "dynamic_algorithms/tc.hpp"

#include "static_algorithms/bfs.hpp"
#include "static_algorithms/cc.hpp"
#include "static_algorithms/kcore.hpp"

struct alg_params {
  uint64_t count;
  uint64_t src;
  uint64_t total_nodes;
  std::string bfs_histogram_filepath;
};

using appendAlgorithms = type_list_tt<append_cc, append_tc, append_bfs
                                      // add append only algorithms to run here
                                      >;

using dynamicAlgorithms = type_list_tt<dynamic_cc, dynamic_tc
                                       // add dynamic algorithms to run here
                                       >;

template <typename Graph>
using StaticAlgorithms =
    type_list<static_bfs<Graph>, static_cc<Graph>, static_kcore<Graph>
              // add static algorithms to run here
              >;

#ifndef NODE_TYPE
#define NODE_TYPE uint32_t
#endif

#ifndef TIMESTAMP_TYPE
#define TIMESTAMP_TYPE uint32_t
#endif

template <class node_t = uint32_t, class timestamp_t = uint32_t>
void graph_statistics(const auto &edges, uint64_t print_freq,
                      std::string &output_dir, node_t src,
                      const alg_params &params) {
  using graph_type = AppendOnlyGraph<node_t, appendAlgorithms>;

  // Ensure the output directory exists
  std::filesystem::create_directories(output_dir);

  std::ofstream stats_file(std::filesystem::path(output_dir) /
                           "statistics.csv");
  graph_type g(params);
  stats_file << "timestep, timestamp, num_nodes, num_edges, average_degree, "
                "max_degree,"
                "new_edges,"
             << g.dynamic_stats_header() << std::endl;

  uint64_t static_alg_freq = std::numeric_limits<uint64_t>::max();
  std::ofstream static_file;
  if (params.count > 0) {
    static_alg_freq = edges.size() / params.count;
    static_file.open(std::filesystem::path(output_dir) / "static.csv");
    static_file << "timestep, timestamp, "
                << csv_header(StaticAlgorithms<graph_type>{}) << std::endl;
  }

  uint64_t i = 0;
  for (; i < edges.size(); i += print_freq) {
    uint64_t count_new = 0;
    uint64_t end = i + print_freq;
    if (end > edges.size()) {
      end = edges.size();
    }
    for (uint64_t j = i; j < end; j++) {
      count_new += g.add_edge(std::get<0>(edges[j]), std::get<1>(edges[j]));
      if ((j + 1) % static_alg_freq == 0) {
        static_file << j + 1 << ", " << std::get<2>(edges[j]) << ",";
        static_file << csv_row(StaticAlgorithms<graph_type>{}, g, params)
                    << std::endl;
      }
    }
    stats_file << end << "," << std::get<2>(edges[i]) << "," << g.num_nodes()
               << "," << g.num_edges() << "," << g.average_degree() << ","
               << g.max_degree() << "," << count_new << ", "
               << g.dynamic_stats() << std::endl;
  }
  stats_file.close();
  if (params.count > 0) {
    static_file.close();
  }
}

template <class node_t = uint32_t, class timestamp_t = uint32_t>
void graph_statistics_remove(uint64_t num_nodes, const auto &edges,
                             uint64_t print_freq, std::string &output_dir,
                             const alg_params &params) {
  using graph_type = Graph<node_t, dynamicAlgorithms>;
  // Ensure the output directory exists
  std::filesystem::create_directories(output_dir);

  std::ofstream stats_file(std::filesystem::path(output_dir) /
                           "statistics.csv");
  graph_type g(params);
  std::cout << "Finished Constructor with n=" << num_nodes << "." << std::endl;
  stats_file << "timestep, timestamp, num_nodes, num_edges, average_degree, "
                "max_degree, "
                "new_edges, "
             << g.dynamic_stats_header() << std::endl;

  uint64_t static_alg_freq = std::numeric_limits<uint64_t>::max();
  std::ofstream static_file;
  if (params.count > 0) {
    static_alg_freq = edges.size() / params.count;
    static_file.open(std::filesystem::path(output_dir) / "static.csv");
    static_file << "timestep, timestamp, "
                << csv_header(StaticAlgorithms<graph_type>{}) << std::endl;
  }

  for (uint64_t i = 0; i < edges.size(); i += print_freq) {
    uint64_t count_new = 0;
    uint64_t end = i + print_freq;
    if (end > edges.size()) {
      end = edges.size();
    }
    for (uint64_t j = i; j < end; j++) {
      if (j % 10000000 == 0)
        std::cout << "Update " << j << " out of " << edges.size() << std::endl;
      if (std::get<2>(edges[j]) == IO::UpdateInfo::INSERT) {
        // std::cout << "insert " << std::get<0>(edges[j]) << ", " <<
        // std::get<1>(edges[j]) << "\n";
        count_new += g.add_edge(std::get<0>(edges[j]), std::get<1>(edges[j]));
      } else {
        // std::cout << "remove " << std::get<0>(edges[j]) << ", " <<
        // std::get<1>(edges[j]) << "\n";
        g.remove_edge(std::get<0>(edges[j]), std::get<1>(edges[j]));
      }
      if ((j + 1) % static_alg_freq == 0) {
        static_file << j + 1 << ", " << std::get<3>(edges[j]) << ",";
        static_file << csv_row(StaticAlgorithms<graph_type>{}, g, params)
                    << std::endl;
      }
    }
    stats_file << end << "," << std::get<3>(edges[i]) << "," << g.num_nodes()
               << "," << g.num_edges() << "," << g.average_degree() << ","
               << g.max_degree() << "," << count_new << "," << g.dynamic_stats()
               << std::endl;
  }
  stats_file.close();
  if (params.count > 0) {
    static_file.close();
  }
}

ABSL_FLAG(std::string, output, "del", "output directory");
ABSL_FLAG(uint64_t, print_freq, 1000, "How often to dump stats to file");
ABSL_FLAG(std::string, input, "", "input filename");
ABSL_FLAG(bool, edges_shuffle, false, "Shuffle the order for edges");
ABSL_FLAG(uint64_t, src, 1, "src to run the bfs from");
ABSL_FLAG(int64_t, max_updates, -1, "max updates to read");
ABSL_FLAG(uint64_t, static_algorithm_count, 0,
          "how many times to run the static algorithms");
ABSL_FLAG(uint64_t, window_size, 0, "window_size");

int main(int32_t argc, char *argv[]) {
  absl::SetProgramUsageMessage("Generate Statistics about a graph");
  absl::ParseCommandLine(argc, argv);
  std::string output_dir = absl::GetFlag(FLAGS_output);
  uint64_t print_freq = absl::GetFlag(FLAGS_print_freq);
  uint64_t window_size = absl::GetFlag(FLAGS_window_size);

  IO::GraphHeader header(absl::GetFlag(FLAGS_input));

  alg_params params;
  params.count = absl::GetFlag(FLAGS_static_algorithm_count);
  params.src = absl::GetFlag(FLAGS_src);
  params.bfs_histogram_filepath =
      std::filesystem::path(output_dir) / "bfs_update_sizes_histogram";

  if (header.if_signal_graph) {
    auto [edges, num_nodes] =
        IO::get_unweighted_graph_edges_signal<NODE_TYPE, TIMESTAMP_TYPE>(
            absl::GetFlag(FLAGS_input), absl::GetFlag(FLAGS_edges_shuffle),
            absl::GetFlag(FLAGS_max_updates));
    params.total_nodes = num_nodes;
    std::cout << "done loading the graph" << std::endl;
    std::cout << "have " << edges.size() << " edges and " << num_nodes
              << " nodes" << std::endl;
    std::cout << " printing every " << print_freq << std::endl;
    if (num_nodes > 0) {
      std::cout << "output filename is " << output_dir << std::endl;
      if (window_size == 0) {
        graph_statistics(edges, print_freq, output_dir,
                         (NODE_TYPE)absl::GetFlag(FLAGS_src), params);
      } else {
        auto windowed_edges = IO::turn_signal_into_windowed(edges, window_size);
        graph_statistics_remove(num_nodes, windowed_edges, print_freq,
                                output_dir, params);
      }
    }
  } else {
    auto [edges, num_nodes] =
        IO::get_unweighted_graph_edges<NODE_TYPE, TIMESTAMP_TYPE>(
            absl::GetFlag(FLAGS_input), absl::GetFlag(FLAGS_edges_shuffle),
            absl::GetFlag(FLAGS_max_updates));
    params.total_nodes = num_nodes;
    std::cout << "done loading the graph" << std::endl;
    std::cout << "have " << edges.size() << " edges and " << num_nodes
              << " nodes" << std::endl;
    std::cout << " printing every " << print_freq << std::endl;
    if (num_nodes > 0) {
      std::cout << "output filename is " << output_dir << std::endl;
      graph_statistics_remove(num_nodes, edges, print_freq, output_dir, params);
    }
  }

  return 0;
}
