#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "graph.hpp"
#include "graph_generators.hpp"
#include "io_util.hpp"
#include <cstdlib>
#include <iostream>
#include <random>
#include <tuple>

template <class edge_t>
void edge_statistics(const std::vector<edge_t> &edges,
                     const std::string &output_filename) {

  using node_t = std::tuple_element_t<0, edge_t>;

  absl::flat_hash_map<std::pair<node_t, node_t>, uint64_t> edge_map;

  for (uint64_t i = 0; i < edges.size(); i++) {
    edge_map[{std::get<0>(edges[i]), std::get<1>(edges[i])}]++;
  }
  std::vector<uint64_t> counts(10);
  for (const auto &e : edge_map) {
    if (e.second > counts.size() + 1) {
      counts.resize(e.second + 1);
    }
    counts[e.second]++;
  }

  std::ofstream edge_count_histogram;
  edge_count_histogram.open(output_filename);
  for (uint64_t i = 0; i < counts.size(); i++) {
    edge_count_histogram << i << ", " << counts[i] << "\n";
  }
  edge_count_histogram.close();
}

ABSL_FLAG(std::string, command, "", "Which test to run");
ABSL_FLAG(std::string, output, "del.csv", "output filename");

ABSL_FLAG(std::string, input, "", "input filename for real graphs");
ABSL_FLAG(bool, remove, false, "If there can be edge removals in the graph");

int main(int32_t argc, char *argv[]) {
  absl::SetProgramUsageMessage(
      "Generate Statistics about edges for a real graph");
  absl::ParseCommandLine(argc, argv);
  std::string output_filename = absl::GetFlag(FLAGS_output);
  if (!absl::GetFlag(FLAGS_remove)) {
    auto [edges, graph_nodes] =
        AppendOnlyGraph<uint32_t>::get_graph_edges(absl::GetFlag(FLAGS_input));
    std::cout << "done loading the graph" << std::endl;
    std::cout << "have " << edges.size() << " edges and " << graph_nodes
              << " nodes" << std::endl;

    if (graph_nodes > 0) {
      std::cout << "output filename is " << output_filename << std::endl;
      edge_statistics(edges, output_filename);
    }
  } else {
    auto [edges, graph_nodes] =
        Graph<uint32_t>::get_graph_edges(absl::GetFlag(FLAGS_input));
    std::cout << "done loading the graph" << std::endl;
    std::cout << "have " << edges.size() << " edges and " << graph_nodes
              << " nodes" << std::endl;

    if (graph_nodes > 0) {
      std::cout << "output filename is " << output_filename << std::endl;
      edge_statistics(edges, output_filename);
    }
  }
  return 0;
}
