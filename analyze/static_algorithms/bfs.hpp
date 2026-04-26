#pragma once
#include "benchmarks/BFS/NonDeterministicBFS/BFS.h"

#include <parlay/primitives.h>

template <typename Graph> struct static_bfs {

  static constexpr std::string_view header = "max_distance_from_src";

  template <typename params>
  static std::string run(const Graph &g, const params &p) {
    auto parents = gbbs::BFS(g, p.src);
    auto max_distaice = parlay::reduce_max(
        parlay::delayed_tabulate(parents.size(), [&parents](size_t i) {
          if (parents[i] == i || parents[i] == UINT_E_MAX) {
            return 0UL;
          }
          size_t distance = 1;
          size_t parent = parents[i];
          while (parent != parents[parent]) {
            distance += 1;
            parent = parents[parent];
          }
          return distance;
        }));
    return std::to_string(max_distaice);
  }
};
