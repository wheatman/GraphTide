#pragma once
#include "benchmarks/KCore/JulienneDBS17/KCore.h"

#include <parlay/primitives.h>

template <typename Graph> struct static_kcore {

  static constexpr std::string_view header =
      "avg_coreness, max_coreness, 25\%_coreness, 50\%_coreness, "
      "75\%_coreness"sv;

  template <typename params>
  static std::string run(const Graph &g, const params &p) {
    auto coreness = gbbs::KCore(g);
    parlay::integer_sort_inplace(coreness);
    auto average = parlay::reduce(coreness) / coreness.size();
    auto max_coreness = coreness.back();
    auto coress_25 = coreness[.25 * coreness.size()];
    auto coress_50 = coreness[.50 * coreness.size()];
    auto coress_75 = coreness[.75 * coreness.size()];

    return std::to_string(average) + "," + std::to_string(max_coreness) + "," +
           std::to_string(coress_25) + "," + std::to_string(coress_50) + "," +
           std::to_string(coress_75);
  }
};
