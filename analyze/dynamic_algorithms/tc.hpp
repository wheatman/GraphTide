#pragma once

// This graph is the underlying graph datastructure
//  for now its just a std::vector<absl::flat_hash_map<node_t, uint32_t>>
template <typename Graph_container> struct dynamic_tc {
  using node_t = Graph_container::value_type::key_type;

  static constexpr std::string_view header = "num_triangles";

  dynamic_tc(const Graph_container &g_) : g(g_) {}

  template <typename params> void set_params(const params &p) {}

  // this is called when the vertex goes from zero degree to non zero degree
  void add_vertex(node_t vertex_id) {}

  // this is called when a vertex goes from non zero degree to zero degree
  void remove_vertex(node_t vertex_id) {}

  // this is called when the edge is added and it didn't already exist
  void add_edge(node_t source, node_t dest) {
    if (g[source].size() <= g[dest].size()) {
      for (const auto &it : g[source]) {
        if (g[dest].contains(it.first)) {
          triangle_count_ += 1;
        }
      }
    } else {
      for (const auto &it : g[dest]) {
        if (g[source].contains(it.first)) {
          triangle_count_ += 1;
        }
      }
    }
  }

  // this is called when the edge is removed and it previously existed
  void remove_edge(node_t source, node_t dest) {
    if (g[source].size() <= g[dest].size()) {
      for (const auto &it : g[source]) {
        if (g[dest].contains(it.first)) {
          triangle_count_ -= 1;
        }
      }
    } else {
      for (const auto &it : g[dest]) {
        if (g[source].contains(it.first)) {
          triangle_count_ -= 1;
        }
      }
    }
  }

  string get_string_values() const { return std::to_string(triangle_count_); }

private:
  const Graph_container &g;
  uint64_t triangle_count_ = 0;
};
