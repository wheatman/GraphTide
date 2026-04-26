#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "graph.hpp"
#include "graph_generators.hpp"
#include "io_util.hpp"
#include <cstdlib>
#include <iostream>
#include <random>

template <class node_t = uint32_t, class timestamp_t = uint32_t>
void node_statistics(
    const std::vector<std::tuple<node_t, node_t, timestamp_t>> &edges,
    uint64_t print_freq, std::string &output_filename,
    const std::vector<node_t> nodes_to_track, bool use_real_time) {
  std::ofstream degree_file;
  degree_file.open(output_filename);
  AppendOnlyGraph<node_t> g;
  degree_file << "timestep,";
  for (const auto &node : nodes_to_track) {
    degree_file << node << ",";
  }
  degree_file << std::endl;

  uint64_t i = 0;
  for (; i < edges.size(); i += print_freq) {
    uint64_t end = i + print_freq;
    if (end > edges.size()) {
      end = edges.size();
    }
    for (uint64_t j = i; j < end; j++) {
      g.add_edge(std::get<0>(edges[j]), std::get<1>(edges[j]));
    }
    if (use_real_time) {
      degree_file << std::get<2>(edges[i]) << ",";
    } else {
      degree_file << end << ",";
    }
    for (const auto &node : nodes_to_track) {
      degree_file << g.degree(node) << ",";
    }
    degree_file << std::endl;
  }
  degree_file.close();
}

template <class node_t = uint32_t, class timestamp_t = uint32_t>
std::vector<node_t> get_nodes_to_track(
    const std::vector<std::tuple<node_t, node_t, timestamp_t>> &edges,
    const std::string command, node_t graph_nodes, size_t num_track) {
  if (command == "random") {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<node_t> dis(0, graph_nodes);
    std::vector<node_t> nodes;
    for (size_t i = 0; i < num_track; i++) {
      nodes.push_back(dis(gen));
    }
    return nodes;
  } else if (command == "top_update") {
    std::vector<std::pair<node_t, size_t>> counts(graph_nodes);
    for (const auto &edge : edges) {
      node_t src = std::get<0>(edge);
      counts[src] = {counts[src].first + 1, src};
      node_t dest = std::get<1>(edge);
      counts[dest] = {counts[dest].first + 1, dest};
    }
    std::partial_sort(counts.begin(), counts.begin() + num_track, counts.end(),
                      std::greater{});

    std::vector<node_t> nodes;
    for (size_t i = 0; i < num_track; i++) {
      nodes.push_back(counts[i].second);
    }
    return nodes;
  } else if (command == "top_degree") {
    AppendOnlyGraph<node_t> g;
    for (const auto &edge : edges) {
      g.add_edge(std::get<0>(edge), std::get<1>(edge));
    }
    std::vector<std::pair<node_t, size_t>> counts(graph_nodes);
    for (size_t i = 0; i < graph_nodes; i++) {
      counts[i] = {g.degree(i), i};
    }
    std::partial_sort(counts.begin(), counts.begin() + num_track, counts.end(),
                      std::greater{});

    std::vector<node_t> nodes;
    for (size_t i = 0; i < num_track; i++) {
      nodes.push_back(counts[i].second);
    }
    return nodes;
  } else {
    std::cout << "unknown command" << std::endl;
    return {};
  }
}

ABSL_FLAG(std::string, command, "", "Which test to run");
ABSL_FLAG(std::string, output, "del.csv", "output filename");
ABSL_FLAG(uint64_t, print_freq, 1000, "How often to dump stats to file");
ABSL_FLAG(uint64_t, num_nodes, 100, "How many nodes to track");
ABSL_FLAG(std::string, input, "", "input filename for real graphs");
ABSL_FLAG(bool, remove, false, "If there can be edge removals in the graph");

ABSL_FLAG(bool, use_real_time, false,
          "If plots should use timestamp or timestep");

int main(int32_t argc, char *argv[]) {
  absl::SetProgramUsageMessage(
      "Generate Statistics about nodes for a real graph");
  absl::ParseCommandLine(argc, argv);
  std::string output_filename = absl::GetFlag(FLAGS_output);
  uint64_t print_freq = absl::GetFlag(FLAGS_print_freq);
  if (!absl::GetFlag(FLAGS_remove)) {
    auto [edges, graph_nodes] =
        AppendOnlyGraph<uint32_t>::get_graph_edges(absl::GetFlag(FLAGS_input));
    std::cout << "done loading the graph" << std::endl;
    std::cout << "have " << edges.size() << " edges and " << graph_nodes
              << " nodes" << std::endl;
    std::cout << " printing every " << print_freq << std::endl;
    if (graph_nodes > 0) {
      auto nodes_to_track =
          get_nodes_to_track(edges, absl::GetFlag(FLAGS_command), graph_nodes,
                             absl::GetFlag(FLAGS_num_nodes));
      std::cout << "output filename is " << output_filename << std::endl;
      if (nodes_to_track.size() > 0) {
        node_statistics(edges, print_freq, output_filename, nodes_to_track,
                        absl::GetFlag(FLAGS_use_real_time));
      }
    }
  } else {
    std::cout << "Not implemented yet" << std::endl;
  }
  return 0;
}
