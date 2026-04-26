#pragma once

#include "SCCWN.hpp"

// This graph is the underlying graph datastructure
//  for now its just a std::vector<absl::flat_hash_map<node_t, uint32_t>>
template <typename Graph_container> struct dynamic_cc {
  using node_t = Graph_container::value_type::key_type;

  static constexpr std::string_view header =
      "component_count, max_component_size";

  dynamic_cc(const Graph_container &g_) {}

  template <typename params> void set_params(const params &p) {
    dynconn = std::make_unique<SCCWN<>>(p.total_nodes);
    for (uint64_t i = 0; i < p.total_nodes; i++)
      component_sizes.insert(1);
  }

  // this is called the first time an edge is added to this vertex
  void add_vertex(node_t vertex_id) { component_count_++; }
  // this is called when a vertex goes from non zero degree to zero degree
  void remove_vertex(node_t vertex_id) { component_count_--; }

  // this is called the first time this edge is added to the graph
  void add_edge(node_t source, node_t dest) {
    uint64_t size_src = dynconn->get_component_size(source);
    uint64_t size_dst = dynconn->get_component_size(dest);
    bool joined = dynconn->insert(source, dest);
    if (joined) {
      component_sizes.erase(component_sizes.find(size_src));
      component_sizes.erase(component_sizes.find(size_dst));
      component_sizes.insert(size_src + size_dst);
      component_count_--;
    }
  }

  // this is called when the edge is removed and it previously existed
  void remove_edge(node_t source, node_t dest) {
    uint64_t size_before = dynconn->get_component_size(source);
    bool split = dynconn->remove(source, dest);
    if (split) {
      component_sizes.erase(component_sizes.find(size_before));
      component_sizes.insert(dynconn->get_component_size(source));
      component_sizes.insert(dynconn->get_component_size(dest));
      component_count_++;
    }
  }

  string get_string_values() const {
    return std::to_string(component_count_) + "," +
           std::to_string(*component_sizes.rbegin());
  }

private:
  std::unique_ptr<SCCWN<>> dynconn = nullptr;
  std::vector<node_t> components;
  absl::btree_multiset<uint64_t> component_sizes;
  uint64_t component_count_ = 0;
};
