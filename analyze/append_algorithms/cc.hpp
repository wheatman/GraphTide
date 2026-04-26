#pragma once
#include <string>

// This graph is the underlying graph datastructure
//  for now its just a std::vector<absl::flat_hash_set<node_t>>
template <typename Graph_container> struct append_cc {
  using node_t = Graph_container::value_type::value_type;

  static constexpr std::string_view header =
      "component_count, max_component_size";

  append_cc(const Graph_container &g_) {}

  template <typename params> void set_params(const params &p) {
    components.resize(p.total_nodes);
    component_sizes.resize(p.total_nodes);
  }

  // this is called the first time an edge is added to this vertex
  void add_vertex(node_t vertex_id) {
    components[vertex_id] = vertex_id;
    component_sizes[vertex_id] = 1;
    max_component_size_ =
        std::max(component_sizes[vertex_id], max_component_size_);
    num_components_ += 1;
  }

  // this is called the first time this edge is added to the graph
  void add_edge(node_t source, node_t dest) {
    node_t root_source = get_component(source);
    node_t root_dest = get_component(dest);
    if (root_source != root_dest) {
      num_components_ -= 1;
      node_t max_v = std::max(root_source, root_dest);
      node_t min_v = std::min(root_source, root_dest);
      components[max_v] = min_v;
      component_sizes[min_v] += component_sizes[max_v];
      max_component_size_ =
          std::max(component_sizes[min_v], max_component_size_);
    }
  }

  std::string get_string_values() const {
    return std::to_string(num_components_) + "," +
           std::to_string(max_component_size_);
  }

private:
  std::vector<node_t> components;
  std::vector<uint64_t> component_sizes;
  uint64_t max_component_size_ = 0;
  uint64_t num_components_ = 0;

  node_t get_component(node_t i) {
    if (i != components[i]) {
      components[i] = get_component(components[i]);
      return components[i];
    }
    return i;
  }
};
