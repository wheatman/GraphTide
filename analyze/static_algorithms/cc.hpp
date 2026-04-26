#pragma once
#include "benchmarks/Connectivity/SimpleUnionAsync/Connectivity.h"

#include <parlay/primitives.h>

std::pair<uint64_t, uint64_t> pair_zero() { return {0, 0}; }
std::pair<uint64_t, uint64_t>
pair_second_max(std::pair<uint64_t, size_t> l,
                const std::pair<uint64_t, size_t> &r) {
  return {0, std::max(std::get<1>(l), std::get<1>(r))};
}

auto pair_max = parlay::binary_op(pair_second_max, pair_zero());

template <typename Graph> struct static_cc {

  static constexpr std::string_view header =
      "component_count, non_empty_components, max_component_size"sv;

  template <typename params>
  static std::string run(const Graph &g, const params &p) {
    auto labels = gbbs::simple_union_find::SimpleUnionAsync(g);
    auto histogram = parlay::histogram_by_key(labels);
    uint64_t num_cc = histogram.size();
    auto non_empty_components =
        parlay::count_if(histogram, [](auto kv) { return kv.second > 1; });
    uint64_t largest_cc = std::get<1>(parlay::reduce(histogram, pair_max));
    return std::to_string(num_cc) + "," + std::to_string(non_empty_components) +
           "," + std::to_string(largest_cc);
  }
};
