#pragma once
#include "graph_generators.hpp"
#include "helpers.hpp"
#include "io_util.hpp"

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include <gbbs/gbbs.h>
#include <parlay/primitives.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

template <class node_t, typename Algs = emptyTypeList> class AppendOnlyGraph {
private:
  using Graph_container = std::vector<absl::flat_hash_set<node_t>>;
  Graph_container graph;
  using AlgsTuple =
      typename make_tuple_from_typelist<Graph_container, Algs>::type;
  AlgsTuple algs_;

  uint64_t num_edges_ = 0;
  uint64_t num_nodes_ = 0;
  uint64_t max_degree_ = 0;

  uint64_t source = std::numeric_limits<uint64_t>::max();

public:
  AppendOnlyGraph() = default;
  AppendOnlyGraph(auto &params)
      : algs_(make_tuple_from_typelist<Graph_container, Algs>::make(graph)) {
    std::apply(
        [&params](auto &&...alg) {
          ((alg.set_params(params)),
           ...); // Fold expression to call print_element for each arg
        },
        algs_);
  }
  // returns true if the edge was added, false if it was already in the graph
  bool add_edge(node_t source, node_t dest) {
    if (source == dest) {
      return false;
    }
    auto max_node = std::max(source, dest);
    if (max_node >= graph.size()) {
      graph.resize(max_node + 1);
    }
    if (graph[source].size() == 0) {
      num_nodes_ += 1;
      std::apply(
          [&](auto &&...alg) {
            ((alg.add_vertex(source)),
             ...); // Fold expression to call print_element for each arg
          },
          algs_);
    }
    if (graph[dest].size() == 0) {
      num_nodes_ += 1;
      std::apply(
          [&](auto &&...alg) {
            ((alg.add_vertex(dest)),
             ...); // Fold expression to call print_element for each arg
          },
          algs_);
    }
    bool inserted = graph[source].insert(dest).second;
    graph[dest].insert(source);
    if (inserted) {
      num_edges_ += 1;
      max_degree_ = std::max((uint64_t)graph[source].size(), max_degree_);
      std::apply(
          [&](auto &&...alg) {
            ((alg.add_edge(source, dest)),
             ...); // Fold expression to call print_element for each arg
          },
          algs_);
    }
    return inserted;
  }

  std::string dynamic_stats() const {
    std::string out;
    std::apply(
        [&](auto const &...alg) {
          std::size_t i = 0;
          ((out += std::string(i++ == 0 ? "" : ",") + alg.get_string_values()),
           ...);
        },
        algs_);

    return out;
  }

  std::string dynamic_stats_header() const {
    std::string out;
    std::apply(
        [&](auto const &...alg) {
          std::size_t i = 0;
          ((out += std::string(i++ == 0 ? "" : ",") + std::string(alg.header)),
           ...);
        },
        algs_);

    return out;
  }

  uint64_t num_nodes() const { return num_nodes_; }
  uint64_t num_edges() const { return num_edges_; }

  uint64_t degree(node_t i) const {
    if (i >= graph.size()) {
      return 0;
    }
    return graph[i].size();
  }
  uint64_t in_degree(node_t i) const { return degree(i); }
  uint64_t out_degree(node_t i) const { return degree(i); }
  uint64_t N() const { return graph.size(); }
  using weight_type = gbbs::empty;
  template <class F> void map_out_neighbors(size_t i, F f) const {
    for (const auto &other : graph[i]) {
      weight_type w{};
      f(i, other, w);
    }
  }
  template <class F> void parallel_map_out_neighbors(size_t i, F f) const {
    return map_out_neighbors(i, f);
  }

  template <class F> void map_in_neighbors(size_t i, F f) const {
    for (const auto &other : graph[i]) {
      weight_type w{};
      f(other, i, w);
    }
  }
  template <class F> void parallel_map_in_neighbors(size_t i, F f) const {
    return map_in_neighbors(i, f);
  }
  template <class F> void map_out_neighbors_early_exit(size_t i, F f) const {
    for (const auto &other : graph[i]) {
      weight_type w{};
      if (f(i, other, w)) {
        break;
      }
    }
  }
  template <class F, typename Block_F = std::nullptr_t>
  void parallel_map_out_neighbors_early_exit(size_t i, F f,
                                             Block_F block_check = {}) const {
    return map_out_neighbors_early_exit(i, f);
  }
  template <class F> void map_in_neighbors_early_exit(size_t i, F f) const {
    for (const auto &other : graph[i]) {
      weight_type w{};
      if (f(other, i, w)) {
        break;
      }
    }
  }
  template <class F, typename Block_F = std::nullptr_t>
  void parallel_map_in_neighbors_early_exit(size_t i, F f,
                                            Block_F block_check = {}) const {
    return map_in_neighbors_early_exit(i, f);
  }

  template <class F> void map_neighbors(size_t i, F f) const {
    weight_type w{};
    for (const auto &other : graph[i]) {
      f(i, other, w);
    }
  }

  template <class F>
  size_t count_in_neighbors(size_t id, F f,
                            [[maybe_unused]] bool unused = true) const {
    size_t count = 0;
    map_in_neighbors(id, [&](auto &...args) { count += f(args...); });
    return count;
  }

  template <class F>
  size_t count_out_neighbors(size_t id, F f,
                             [[maybe_unused]] bool unused = true) const {
    size_t count = 0;
    map_out_neighbors(id, [&](auto &...args) { count += f(args...); });
    return count;
  }

  uint64_t M() const { return num_edges(); }

  double average_degree() const {
    return static_cast<double>(num_edges()) / num_nodes();
  }
  uint64_t max_degree() const { return max_degree_; }

  template <class timestamp_t = uint32_t>
  static std::pair<std::vector<std::tuple<node_t, node_t, timestamp_t>>, node_t>
  get_graph_edges(const std::string &command, node_t num_nodes,
                  uint64_t rmat_num_edges, double rmat_a, double rmat_b,
                  double rmat_c, double er_p, uint64_t ws_K, double ws_beta,
                  const std::string &input_filename, bool shuffle) {
    std::vector<std::tuple<node_t, node_t, timestamp_t>> edges;
    if (command == "rmat") {
      edges = generate_rmat(num_nodes, rmat_num_edges, rmat_a, rmat_b, rmat_c);
    } else if (command == "er") {
      edges = generate_er(num_nodes, er_p);
    } else if (command == "ws") {
      edges = generate_watts_strogatz(num_nodes, ws_K, ws_beta);
    } else if (command == "adj") {
      uint64_t num_edges;
      edges =
          get_edges_from_file_adj_sym(input_filename, &num_nodes, &num_edges);
    } else if (command == "adj_rmat") {
      uint64_t num_edges;
      edges =
          get_edges_from_file_adj_sym(input_filename, &num_nodes, &num_edges);
      auto edges2 =
          generate_rmat(num_nodes, rmat_num_edges, rmat_a, rmat_b, rmat_c);
      edges.insert(edges.end(), edges2.begin(), edges2.end());
    } else if (command == "adj_er") {
      uint64_t num_edges;
      edges =
          get_edges_from_file_adj_sym(input_filename, &num_nodes, &num_edges);
      auto edges2 = generate_er(
          num_nodes, static_cast<double>(num_edges) /
                         (static_cast<uint64_t>(num_nodes) * num_nodes));
      edges.insert(edges.end(), edges2.begin(), edges2.end());
    } else if (command == "adj_ws") {
      uint64_t num_edges;
      edges =
          get_edges_from_file_adj_sym(input_filename, &num_nodes, &num_edges);
      auto edges2 =
          generate_watts_strogatz(num_nodes, num_edges / num_nodes, ws_beta);
      edges.insert(edges.end(), edges2.begin(), edges2.end());
    } else if (command == "edges") {
      edges = get_edges_from_file_edges<node_t>(input_filename, shuffle);
      for (const auto &e : edges) {
        num_nodes = std::max(num_nodes, std::get<0>(e));
        num_nodes = std::max(num_nodes, std::get<1>(e));
      }
    } else if (command == "edges_bin") {
      edges = get_binary_edges_from_file_edges(input_filename, shuffle);
      for (const auto &e : edges) {
        num_nodes = std::max(num_nodes, std::get<0>(e));
        num_nodes = std::max(num_nodes, std::get<1>(e));
      }
    } else {
      std::cout << "command not found was: " << command << std::endl;
      num_nodes = 0;
    }

    return {std::move(edges), num_nodes + 1};
  }
  template <class timestamp_t = uint32_t>
  static std::pair<std::vector<std::tuple<node_t, node_t, timestamp_t>>, node_t>
  get_graph_edges(const std::string &input_filename) {
    if (ends_with(input_filename, "bin")) {
      return get_graph_edges("edges_bin", 0, 0, 0, 0, 0, 0, 0, 0,
                             input_filename, false);
    }
    return get_graph_edges("edges", 0, 0, 0, 0, 0, 0, 0, 0, input_filename,
                           false);
  }
};

template <class node_t = uint32_t, typename Algs = emptyTypeList> class Graph {
private:
  using Graph_container = std::vector<absl::flat_hash_map<node_t, uint32_t>>;
  using AlgsTuple =
      typename make_tuple_from_typelist<Graph_container, Algs>::type;
  Graph_container graph;
  AlgsTuple algs_;

  uint64_t num_edges_ = 0;
  uint64_t num_nodes_ = 0;
  // degree, id
  absl::btree_set<std::pair<node_t, node_t>> degree_heap;

public:
  // Constructor
  // Graph() = default;
  Graph(auto &params)
      : algs_(make_tuple_from_typelist<Graph_container, Algs>::make(graph)) {
    graph.resize(params.total_nodes);
    std::apply(
        [&params](auto &&...alg) {
          ((alg.set_params(params)),
           ...); // Fold expression to call print_element for each arg
        },
        algs_);
  }

  // returns true if the edge was added, false if it was already in the graph
  bool add_edge(node_t source, node_t dest) {
    if (source == dest) {
      return false;
    }
    if (graph.size() > source && graph.size() > dest &&
        graph[source].contains(dest)) {
      return false;
    }
    if (graph[source].size() == 0) {
      num_nodes_ += 1;
      std::apply(
          [&](auto &&...alg) {
            ((alg.add_vertex(dest)),
             ...); // Fold expression to call print_element for each arg
          },
          algs_);
    }
    if (graph[dest].size() == 0) {
      num_nodes_ += 1;
      std::apply(
          [&](auto &&...alg) {
            ((alg.add_vertex(dest)),
             ...); // Fold expression to call print_element for each arg
          },
          algs_);
    }
    graph[source][dest] += 1;
    bool inserted = (graph[source][dest] == 1);
    graph[dest][source] += 1;
    if (inserted) {
      num_edges_ += 1;
      degree_heap.erase({graph[source].size() - 1, source});
      degree_heap.erase({graph[dest].size() - 1, dest});
      degree_heap.insert({graph[source].size(), source});
      degree_heap.insert({graph[dest].size(), dest});
      std::apply(
          [&](auto &&...alg) {
            ((alg.add_edge(source, dest)),
             ...); // Fold expression to call print_element for each arg
          },
          algs_);
    }
    return inserted;
  }
  bool remove_edge(node_t source, node_t dest) {
    if (source == dest) {
      return false;
    }
    if (graph.size() <= source || graph.size() <= dest ||
        !graph[source].contains(dest)) {
      return false;
    }
    if (graph[source].size() == 1) {
      num_nodes_ -= 1;
      std::apply(
          [&](auto &&...alg) {
            ((alg.remove_vertex(dest)),
             ...); // Fold expression to call print_element for each arg
          },
          algs_);
    }
    if (graph[dest].size() == 1) {
      num_nodes_ -= 1;
      std::apply(
          [&](auto &&...alg) {
            ((alg.remove_vertex(dest)),
             ...); // Fold expression to call print_element for each arg
          },
          algs_);
    }
    bool contained = graph[source].contains(dest);
    bool removed = false;
    if (contained) {
      graph[source][dest] -= 1;
      graph[dest][source] -= 1;
      if (graph[source][dest] == 0) {
        removed = true;
        graph[source].erase(dest);
        graph[dest].erase(source);
      }
    }

    if (removed) {
      num_edges_ -= 1;
      degree_heap.erase({graph[source].size() + 1, source});
      degree_heap.erase({graph[dest].size() + 1, dest});
      degree_heap.insert({graph[source].size(), source});
      degree_heap.insert({graph[dest].size(), dest});
      std::apply(
          [&](auto &&...alg) {
            ((alg.remove_edge(source, dest)),
             ...); // Fold expression to call print_element for each arg
          },
          algs_);
    }
    return removed;
  }

  std::string dynamic_stats() const {
    std::string out;
    std::apply(
        [&](auto const &...alg) {
          std::size_t i = 0;
          ((out += std::string(i++ == 0 ? "" : ",") + alg.get_string_values()),
           ...);
        },
        algs_);

    return out;
  }

  std::string dynamic_stats_header() const {
    std::string out;
    std::apply(
        [&](auto const &...alg) {
          std::size_t i = 0;
          ((out += std::string(i++ == 0 ? "" : ",") + std::string(alg.header)),
           ...);
        },
        algs_);

    return out;
  }

  uint64_t degree(node_t i) const {
    if (i >= graph.size()) {
      return 0;
    }
    return graph[i].size();
  }
  uint64_t num_nodes() const { return num_nodes_; }
  uint64_t num_edges() const { return num_edges_; }

  uint64_t in_degree(node_t i) const { return degree(i); }
  uint64_t out_degree(node_t i) const { return degree(i); }
  uint64_t N() const { return graph.size(); }
  using weight_type = gbbs::empty;
  template <class F> void map_out_neighbors(size_t i, F f) const {
    for (const auto &other : graph[i]) {
      weight_type w{};
      f(i, other.first, w);
    }
  }
  template <class F> void parallel_map_out_neighbors(size_t i, F f) const {
    return map_out_neighbors(i, f);
  }

  template <class F> void map_in_neighbors(size_t i, F f) const {
    for (const auto &other : graph[i]) {
      weight_type w{};
      f(other.first, i, w);
    }
  }
  template <class F> void parallel_map_in_neighbors(size_t i, F f) const {
    return map_in_neighbors(i, f);
  }
  template <class F> void map_out_neighbors_early_exit(size_t i, F f) const {
    for (const auto &other : graph[i]) {
      weight_type w{};
      if (f(i, other.first, w)) {
        break;
      }
    }
  }
  template <class F, typename Block_F = std::nullptr_t>
  void parallel_map_out_neighbors_early_exit(size_t i, F f,
                                             Block_F block_check = {}) const {
    return map_out_neighbors_early_exit(i, f);
  }
  template <class F> void map_in_neighbors_early_exit(size_t i, F f) const {
    for (const auto &other : graph[i]) {
      weight_type w{};
      if (f(other.first, i, w)) {
        break;
      }
    }
  }
  template <class F, typename Block_F = std::nullptr_t>
  void parallel_map_in_neighbors_early_exit(size_t i, F f,
                                            Block_F block_check = {}) const {
    return map_in_neighbors_early_exit(i, f);
  }

  template <class F> void map_neighbors(size_t i, F f) const {
    weight_type w{};
    for (const auto &other : graph[i]) {
      f(i, other.first, w);
    }
  }

  template <class F>
  size_t count_in_neighbors(size_t id, F f,
                            [[maybe_unused]] bool unused = true) const {
    size_t count = 0;
    map_in_neighbors(id, [&](auto &...args) { count += f(args...); });
    return count;
  }

  template <class F>
  size_t count_out_neighbors(size_t id, F f,
                             [[maybe_unused]] bool unused = true) const {
    size_t count = 0;
    map_out_neighbors(id, [&](auto &...args) { count += f(args...); });
    return count;
  }

  uint64_t M() const { return num_edges(); }
  double average_degree() {
    return static_cast<double>(num_edges()) / num_nodes();
  }
  uint64_t max_degree() {
    if (degree_heap.empty()) {
      return 0;
    }
    return degree_heap.rbegin()->first;
  }

  template <class timestamp_t = uint32_t>
  static std::pair<std::vector<std::tuple<node_t, node_t, bool, timestamp_t>>,
                   node_t>
  get_graph_edges(const std::string &command, node_t num_nodes,
                  uint64_t rmat_num_edges, double rmat_a, double rmat_b,
                  double rmat_c, double er_p, uint64_t ws_K, double ws_beta,
                  const std::string &input_filename, bool shuffle) {
    std::vector<std::tuple<node_t, node_t, bool, timestamp_t>> edges;
    if (command == "edges_bin") {
      edges =
          get_binary_edges_from_file_edges_with_remove(input_filename, shuffle);
      for (const auto &e : edges) {
        num_nodes = std::max(num_nodes, std::get<0>(e));
        num_nodes = std::max(num_nodes, std::get<1>(e));
      }
    } else {
      std::cout << "command not found was: " << command << std::endl;
      num_nodes = 0;
    }

    return {std::move(edges), num_nodes + 1};
  }
  template <class timestamp_t = uint32_t>
  static std::pair<std::vector<std::tuple<node_t, node_t, bool, timestamp_t>>,
                   node_t>
  get_graph_edges(const std::string &input_filename) {
    if (ends_with(input_filename, "bin")) {
      return get_graph_edges("edges_bin", 0, 0, 0, 0, 0, 0, 0, 0,
                             input_filename, false);
    }
    return get_graph_edges("edges", 0, 0, 0, 0, 0, 0, 0, 0, input_filename,
                           false);
  }
};
