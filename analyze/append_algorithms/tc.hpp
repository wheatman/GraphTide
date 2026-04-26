#pragma once
#include <string>

// This graph is the underlying graph datastructure
//  for now its just a std::vector<absl::flat_hash_set<node_t>>
template <typename Graph_container> struct append_tc {
  using node_t = Graph_container::value_type::value_type;

  static constexpr std::string_view header = "num_triangles";

  append_tc(const Graph_container &g_) : g(g_) {}

  template <typename params> void set_params(const params &p) {}

  // this is called the first time an edge is added to this vertex
  void add_vertex(node_t vertex_id) {}

  // this is called the first time this edge is added to the graph
  void add_edge(node_t source, node_t dest) {
    if (g[source].size() <= g[dest].size()) {
      for (const auto &it : g[source]) {
        if (g[dest].contains(it)) {
          triangle_count_ += 1;
        }
      }
    } else {
      for (const auto &it : g[dest]) {
        if (g[source].contains(it)) {
          triangle_count_ += 1;
        }
      }
    }
  }

  std::string get_string_values() const {
    return std::to_string(triangle_count_);
  }

private:
  const Graph_container &g;
  uint64_t triangle_count_ = 0;
};
