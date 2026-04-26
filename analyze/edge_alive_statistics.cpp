#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "graph.hpp"
#include "graph_generators.hpp"
#include "io_util.hpp"
#include <cstdlib>
#include <deque>
#include <iostream>
#include <random>
#include <tuple>

template <class edge_t>
void edge_statistics(std::vector<edge_t> &edges,
                     const std::string &output_filename, uint64_t print_freq,
                     const std::vector<uint64_t> &window_sizes) {

  using node_t = std::tuple_element_t<0, edge_t>;
  using timestamp_t =
      std::tuple_element_t<std::tuple_size_v<edge_t> - 1, edge_t>;

  constexpr bool removals = std::tuple_size_v<edge_t> == 4;
  auto src = [](edge_t &e) -> node_t & { return std::get<0>(e); };
  auto dest = [](edge_t &e) -> node_t & { return std::get<1>(e); };
  auto insert = [](edge_t e) {
    if constexpr (removals) {
      return std::get<2>(e);
    } else {
      return true;
    }
  };
  auto timestamp = [](edge_t &e) -> timestamp_t & {
    if constexpr (removals) {
      return std::get<3>(e);
    } else {
      return std::get<2>(e);
    }
  };

  std::vector<absl::flat_hash_map<std::pair<node_t, node_t>, uint32_t>>
      alive_edges(window_sizes.size());
  absl::flat_hash_set<std::pair<node_t, node_t>> total_edges;

  std::vector<std::deque<edge_t>> remove_after_window;
  remove_after_window.resize(window_sizes.size());

  std::ofstream active_edges_file;
  active_edges_file.open(output_filename);

  active_edges_file << "step count, timestamp";
  for (uint64_t j = 0; j < window_sizes.size(); j++) {
    active_edges_file << ", window of " << window_sizes[j];
  }
  active_edges_file << ", total edges\n";

  for (uint64_t i = 0; i < edges.size(); i++) {
    // add the new edge from the timestamp
    timestamp_t ts = timestamp(edges[i]);
    if (insert(edges[i])) {
      total_edges.insert({src(edges[i]), dest(edges[i])});
      for (uint64_t j = 0; j < window_sizes.size(); j++) {
        auto window_size = window_sizes[j];
        alive_edges[j][{src(edges[i]), dest(edges[i])}]++;
        // std::cout << "edge (" << src(edges[i]) << ", " << dest(edges[i]) << ") has score " << alive_edges[j][{src(edges[i]), dest(edges[i])}] << "\n";
        edge_t windowed_remove = edges[i];
        timestamp(windowed_remove) = timestamp(edges[i]) + window_size;
        remove_after_window[j].push_back(windowed_remove);
      }
    } else {
      for (uint64_t j = 0; j < window_sizes.size(); j++) {
        if (alive_edges[j].contains({src(edges[i]), dest(edges[i])})) {
          alive_edges[j][{src(edges[i]), dest(edges[i])}]--;
          if (alive_edges[j][{src(edges[i]), dest(edges[i])}] == 0) {
            alive_edges[j].erase({src(edges[i]), dest(edges[i])});
          }
        }
      }
    }
    // check what edges have windowed out
    for (uint64_t j = 0; j < window_sizes.size(); j++) {

      while (!remove_after_window[j].empty() &&
             timestamp(remove_after_window[j].front()) <= ts) {
        edge_t e = remove_after_window[j].front();
        if (alive_edges[j].contains({src(e), dest(e)})) {
          alive_edges[j][{src(e), dest(e)}]--;
          if (alive_edges[j][{src(e), dest(e)}] == 0) {
            alive_edges[j].erase({src(e), dest(e)});
          }
        }
        remove_after_window[j].pop_front();
      }
    }
    if (i % print_freq == 0 && i > 0) {
      active_edges_file << i << ", " << ts;
      for (uint64_t j = 0; j < window_sizes.size(); j++) {
        active_edges_file << ", " << alive_edges[j].size();
      }
      active_edges_file << ", " << total_edges.size() << "\n";
    }
  }
  active_edges_file << edges.size() << ", " << timestamp(edges.back());
  for (uint64_t j = 0; j < window_sizes.size(); j++) {
    active_edges_file << ", " << alive_edges[j].size();
  }
  active_edges_file << ", " << total_edges.size() << "\n";
  active_edges_file.close();
}

ABSL_FLAG(std::string, command, "", "Which test to run");
ABSL_FLAG(std::string, output, "del.csv", "output filename");

ABSL_FLAG(std::string, input, "", "input filename for real graphs");
ABSL_FLAG(bool, remove, false, "If there can be edge removals in the graph");
ABSL_FLAG(uint64_t, print_freq, 1000, "How often to dump stats to file");
ABSL_FLAG(std::vector<std::string>, windows,
          std::vector<std::string>({"100", "1000", "10000", "100000", "1000000",
                                    "10000000", "100000000", "1000000000"}),
          "the sizes of the different windows");

int main(int32_t argc, char *argv[]) {
  absl::SetProgramUsageMessage("Generate Statistics about edges for a real "
                               "graph with different size windowing");
  absl::ParseCommandLine(argc, argv);
  std::string output_filename = absl::GetFlag(FLAGS_output);
  uint64_t print_freq = absl::GetFlag(FLAGS_print_freq);
  std::vector<uint64_t> windows;
  for (auto &s_window : absl::GetFlag(FLAGS_windows)) {
    windows.push_back(atoll(s_window.c_str()));
  }
  if (!absl::GetFlag(FLAGS_remove)) {
    auto [edges, graph_nodes] =
        AppendOnlyGraph<uint32_t>::get_graph_edges(absl::GetFlag(FLAGS_input));
    std::cout << "done loading the graph" << std::endl;
    std::cout << "have " << edges.size() << " edges and " << graph_nodes
              << " nodes" << std::endl;

    if (graph_nodes > 0) {
      std::cout << "output filename is " << output_filename << std::endl;
      edge_statistics(edges, output_filename, print_freq, windows);
    }
  } else {
    auto [edges, graph_nodes] =
        Graph<uint32_t>::get_graph_edges(absl::GetFlag(FLAGS_input));
    std::cout << "done loading the graph" << std::endl;
    std::cout << "have " << edges.size() << " edges and " << graph_nodes
              << " nodes" << std::endl;

    if (graph_nodes > 0) {
      std::cout << "output filename is " << output_filename << std::endl;
      edge_statistics(edges, output_filename, print_freq, windows);
    }
  }
  return 0;
}
