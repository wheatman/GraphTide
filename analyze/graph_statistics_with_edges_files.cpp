#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "graph.hpp"
#include "graph_generators.hpp"
#include "io_util.hpp"
#include <cstdlib>
#include <iostream>

template <class node_t = uint32_t, class timestamp_t = uint32_t>
void graph_statistics(
    const std::vector<std::tuple<node_t, node_t, timestamp_t>> &edges,
    uint64_t print_freq, std::string &output_filename, node_t src = 1) {
  std::ofstream myfile;
  myfile.open(output_filename);
  AppendOnlyGraph<> g;
  g.set_bfs_src(src);
  myfile << "timestep, timestamp, num_nodes, num_edges, average_degree, "
            "max_degree, "
            "num_triangles, "
            "new_edges, component_count, max_distance_from_src"
         << std::endl;

  uint64_t i = 0;
  for (; i < edges.size(); i += print_freq) {
    uint64_t count_new = 0;
    uint64_t end = i + print_freq;
    if (end > edges.size()) {
      end = edges.size();
    }
    for (uint64_t j = i; j < end; j++) {
      count_new += g.add_edge(std::get<0>(edges[j]), std::get<1>(edges[j]));
    }
    myfile << end << "," << std::get<2>(edges[i]) << "," << g.num_nodes() << ","
           << g.num_edges() << "," << g.average_degree() << ","
           << g.max_degree() << "," << g.num_triangles() << "," << count_new
           << ", " << g.num_components() << ", " << g.max_distance_from_source()
           << std::endl;
  }
  myfile.close();

  for (auto [size, count] : g.get_bfs_update_sizes()) {
    std::cout << size << ", " << count << "\n";
  }
}

template <class node_t = uint32_t, class timestamp_t = uint32_t>
void graph_statistics_remove(
    const std::vector<std::tuple<node_t, node_t, bool, timestamp_t>> &edges,
    uint64_t print_freq, std::string &output_filename) {
  std::ofstream myfile;
  myfile.open(output_filename);
  Graph<> g;
  myfile << "timestep, timestamp, num_nodes, num_edges, average_degree, "
            "max_degree, "
            "num_triangles, "
            "new_edges"
         << std::endl;

  uint64_t i = 0;
  for (; i < edges.size(); i += print_freq) {
    uint64_t count_new = 0;
    uint64_t end = i + print_freq;
    if (end > edges.size()) {
      end = edges.size();
    }
    for (uint64_t j = i; j < end; j++) {
      if (std::get<2>(edges[j])) {
        count_new += g.add_edge(std::get<0>(edges[j]), std::get<1>(edges[j]));
      } else {
        g.remove_edge(std::get<0>(edges[j]), std::get<1>(edges[j]));
      }
    }
    myfile << end << "," << std::get<3>(edges[i]) << "," << g.num_nodes() << ","
           << g.num_edges() << "," << g.average_degree() << ","
           << g.max_degree() << "," << g.num_triangles() << "," << count_new
           << std::endl;
  }
  myfile.close();
}

ABSL_FLAG(std::string, command, "", "Which test to run");
ABSL_FLAG(std::string, output, "del.csv", "output filename");
ABSL_FLAG(uint64_t, print_freq, 1000, "How often to dump stats to file");
ABSL_FLAG(uint64_t, num_nodes, 0, "Number of Nodes, used in generated graphs");
ABSL_FLAG(uint64_t, rmat_num_edges, 0, "Number of edges to generate for rmat");
ABSL_FLAG(double, rmat_a, .5, "Value of a for rmat");
ABSL_FLAG(double, rmat_b, .1, "Value of b for rmat");
ABSL_FLAG(double, rmat_c, .1, "Value of c for rmat");
ABSL_FLAG(double, er_p, .01, "Value of p for er");
ABSL_FLAG(uint64_t, ws_k, 40, "Value of K for ws");
ABSL_FLAG(double, ws_beta, .1, "Value of beta for ws");
ABSL_FLAG(std::string, input, "", "input filename for real graphs");
ABSL_FLAG(bool, edges_shuffle, false,
          "Shuffle the order for edges in a .edges file");
ABSL_FLAG(bool, remove, false, "If there can be edge removals in the graph");

int main(int32_t argc, char *argv[]) {
  absl::SetProgramUsageMessage(
      "Generate Statistics for a variaty of graph types");
  absl::ParseCommandLine(argc, argv);
  std::string output_filename = absl::GetFlag(FLAGS_output);
  uint64_t print_freq = absl::GetFlag(FLAGS_print_freq);
  if (!absl::GetFlag(FLAGS_remove)) {
    auto [edges, num_nodes] = AppendOnlyGraph<uint32_t>::get_graph_edges(
        absl::GetFlag(FLAGS_command), absl::GetFlag(FLAGS_num_nodes),
        absl::GetFlag(FLAGS_rmat_num_edges), absl::GetFlag(FLAGS_rmat_a),
        absl::GetFlag(FLAGS_rmat_b), absl::GetFlag(FLAGS_rmat_c),
        absl::GetFlag(FLAGS_er_p), absl::GetFlag(FLAGS_ws_k),
        absl::GetFlag(FLAGS_ws_beta), absl::GetFlag(FLAGS_input),
        absl::GetFlag(FLAGS_edges_shuffle));
    std::cout << "done loading the graph" << std::endl;
    std::cout << "have " << edges.size() << " edges and " << num_nodes
              << " nodes" << std::endl;
    std::cout << " printing every " << print_freq << std::endl;
    if (num_nodes > 0) {
      std::cout << "output filename is " << output_filename << std::endl;
      graph_statistics(edges, print_freq, output_filename);
    }
  } else {
    auto [edges, num_nodes] = Graph<uint32_t>::get_graph_edges(
        absl::GetFlag(FLAGS_command), absl::GetFlag(FLAGS_num_nodes),
        absl::GetFlag(FLAGS_rmat_num_edges), absl::GetFlag(FLAGS_rmat_a),
        absl::GetFlag(FLAGS_rmat_b), absl::GetFlag(FLAGS_rmat_c),
        absl::GetFlag(FLAGS_er_p), absl::GetFlag(FLAGS_ws_k),
        absl::GetFlag(FLAGS_ws_beta), absl::GetFlag(FLAGS_input),
        absl::GetFlag(FLAGS_edges_shuffle));
    std::cout << "done loading the graph" << std::endl;
    std::cout << "have " << edges.size() << " edges and " << num_nodes
              << " nodes" << std::endl;
    std::cout << " printing every " << print_freq << std::endl;
    if (num_nodes > 0) {
      std::cout << "output filename is " << output_filename << std::endl;
      graph_statistics_remove(edges, print_freq, output_filename);
    }
  }
  return 0;
}
