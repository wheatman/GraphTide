/** @file dynamic_connectivity.hpp
 *  Declaration for a data structure that maintains connectivity information on
 *  an undirected graph that can have edges added or removed.
 *
 *  @author Tom Tseng (tomtseng)
 */
#pragma once

#include "parlay/parallel.h"
#include <absl/container/btree_set.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <dynamic_forest.hpp>
#include <dynamic_graph/graph.hpp>
#include <utilities/hash.hpp>

namespace detail {

typedef int8_t Level;

enum class EdgeType {
  // Edge is in the spanning forest of the graph.
  kNonTree,
  // Edge is not in the spanning forest of the graph.
  kTree,
};

struct EdgeInfo {
  Level level;
  EdgeType type;
};

} // namespace detail

/** This class represents an undirected graph that can undergo efficient edge
 *  insertions, edge deletions, and connectivity queries. Multiple edges between
 *  a pair of vertices are supported.
 */
class DynamicConnectivity {
public:
  /** Initializes an empty graph with a fixed number of vertices.
   *
   *  Efficiency: \f$ O(n \log n ) \f$ where \f$ n \f$ is the number of vertices
   *  in the graph.
   *
   *  @param[in] num_vertices Number of vertices in the graph.
   */
  explicit DynamicConnectivity(uint32_t num_vertices);

  /** Deallocates the data structure. */
  ~DynamicConnectivity();

  /** The default constructor is invalid because the number of vertices in the
   *  graph must be known. */
  DynamicConnectivity() = delete;
  /** Copy constructor not implemented. */
  DynamicConnectivity(const DynamicConnectivity &other) = delete;
  /** Copy assignment not implemented. */
  DynamicConnectivity &operator=(const DynamicConnectivity &other) = delete;

  /** Move constructor. */
  DynamicConnectivity(DynamicConnectivity &&other) noexcept;
  /** Move assignment not implemented. */
  DynamicConnectivity &operator=(DynamicConnectivity &&other) noexcept;

  /** Returns true if vertices \p u and \p v are connected in the graph.
   *
   *  Efficiency: logarithmic in the size of the graph.
   *
   *  @param[in] u Vertex.
   *  @param[in] v Vertex.
   *  @returns True if \p u and \p v are connected, false if they are not.
   */
  bool IsConnected(Vertex u, Vertex v) const;

  /** Returns true if edge \p edge is in the graph.
   *
   *  Efficiency: constant on average.
   *
   *  @param[in] edge Edge.
   *  @returns True if \p edge is in the graph, false if it is not.
   */
  bool HasEdge(const UndirectedEdge &edge) const;

  /** Returns the number of vertices in `v`'s connected component.
   *
   * Efficiency: logarithmic in the size of the graph.
   *
   * @param[in] v Vertex.
   * @returns The number of vertices in \p v's connected component.
   */
  uint32_t GetSizeOfConnectedComponent(Vertex v) const;

  /** Adds an edge to the graph.
   *
   *  The edge must not already be in the graph and must not be a self-loop
   * edge.
   *
   *  Efficiency: \f$ O\left( \log^2 n \right) \f$ amortized where \f$ n \f$ is
   *  the number of vertices in the graph.
   *
   *  @param[in] edge Edge to be added.
   */
  void AddEdge(const UndirectedEdge &edge);

  /** Deletes an edge from the graph.
   *
   *  An exception will be thrown if the edge is not in the graph.
   *
   *  Efficiency: \f$ O\left( \log^2 n \right) \f$ amortized where \f$ n \f$ is
   *  the number of vertices in the graph.
   *
   *  @param[in] edge Edge to be deleted.
   */
  void DeleteEdge(const UndirectedEdge &edge);
  size_t space() {
    std::atomic<size_t> space = 0;

    space += sizeof(uint32_t);
    space += sizeof(std::vector<DynamicForest>);
    parlay::parallel_for(0, spanning_forests_.size(), [&](size_t i) {
      space += spanning_forests_[i].space();
    });
    space += sizeof(std::vector<std::vector<absl::btree_set<Vertex>>>);
    parlay::parallel_for(0, non_tree_adjacency_lists_.size(), [&](size_t i) {
      space += sizeof(std::vector<absl::btree_set<Vertex>>);
      auto level = non_tree_adjacency_lists_[i];
      parlay::parallel_for(0, level.size(), [&](size_t j) {
        space += sizeof(absl::btree_set<Vertex>);
        space += level[j].size() * 5;
      });
    });
    space += sizeof(absl::flat_hash_map<UndirectedEdge, detail::EdgeInfo,
                                        UndirectedEdgeHash>);
    space += edges_.bucket_count() *
             (sizeof(std::pair<UndirectedEdge, detail::EdgeInfo>));
    return space;
  }
  std::size_t succ_replace;
  std::size_t split;

private:
  void AddNonTreeEdge(const UndirectedEdge &edge);
  void AddTreeEdge(const UndirectedEdge &edge);
  void AddEdgeToAdjacencyList(const UndirectedEdge &edge, detail::Level level);
  void DeleteEdgeFromAdjacencyList(const UndirectedEdge &edge,
                                   detail::Level level);
  void ReplaceTreeEdge(const UndirectedEdge &edge, detail::Level level);

  const uint32_t num_vertices_;
  // `spanning_forests_[i]` stores F_i, the spanning forest for the i-th
  // subgraph. In particular, `spanning_forests[0]` is a spanning forest for the
  // whole graph.
  std::vector<DynamicForest> spanning_forests_;
  // `adjacency_lists_by_level_[i][v]` contains the vertices connected to vertex
  // v by level-i non-tree edges.
  // std::vector<std::vector<absl::flat_hash_set<Vertex>>>
  //     non_tree_adjacency_lists_;
  std::vector<std::vector<absl::btree_set<Vertex>>> non_tree_adjacency_lists_;
  // All edges in the graph.
  absl::flat_hash_map<UndirectedEdge, detail::EdgeInfo, UndirectedEdgeHash>
      edges_;
};
