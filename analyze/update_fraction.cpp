#include "io_util.hpp"
#include "absl/container/flat_hash_map.h"

std::vector<std::tuple<uint32_t, uint32_t, uint32_t>>
get_edges(const char *fileName, uint64_t src_col, uint64_t dest_col,
          uint64_t timestep_col, uint64_t total_cols, bool convert_to_id) {

  long n;
  auto Str = readStringFromFile(fileName, &n);
  auto w = stringToWords(Str, n);
  absl::flat_hash_map<std::string, uint32_t> conversion;
  std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> edges;

  uint64_t id = 0;
  for (uint64_t row = 0; row < w.m / total_cols; row++) {
    if (w.Strings[row * total_cols][0] == '%') {
      continue;
    }
    auto src = w.Strings[row * total_cols + src_col];
    uint32_t src_id = 0;
    auto dest = w.Strings[row * total_cols + dest_col];
    uint32_t dest_id = 0;
    uint64_t timestamp = atoll(w.Strings[row * total_cols + timestep_col]);
    if (convert_to_id) {
      if (!conversion.contains(src)) {
        conversion[src] = id;
        src_id = id;
        id++;
      } else {
        src_id = conversion[src];
      }
      if (!conversion.contains(dest)) {
        conversion[dest] = id;
        dest_id = id;
        id++;
      } else {
        dest_id = conversion[dest];
      }
    } else {
      src_id = atoll(src);
      dest_id = atoll(dest);
    }
    edges.push_back({src_id, dest_id, timestamp});
  }
  return edges;
}


struct pair_hash {
  std::size_t operator()(const std::pair<uint32_t, uint32_t> &pair) const {
    return (((uint64_t)pair.first) << 32UL) | pair.second;
  }
};
/*/
void statistics(const std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> &edges) {
  printf(" num updates = %zu, ", edges.size());
  absl::flat_hash_map<std::pair<uint32_t, uint32_t>, uint32_t, pair_hash> counts;
  uint64_t count = 0;
  for (auto &edge : edges) {
    if (++count % 1000000 == 0) {
      std::cerr << "done " << count << " out of " << edges.size() << "\n";
    }
    counts[{std::get<0>(edge), std::get<1>(edge)}]++;
  }
  printf("num edges = %zu\n", counts.size());
  uint64_t count2 = 0;
  std::map<uint32_t, uint32_t> counts_histogram;
  for (auto count : counts) {
    if (++count2 % 1000000 == 0) {
      std::cerr << "done " << count2 << " out of " << counts.size() << "\n";
    }
    counts_histogram[std::get<1>(count)]++;
  }
  for (auto hist : counts_histogram) {
    std::cout << std::get<0>(hist) << ", " << std::get<1>(hist) << "\n";
  }
  return;
}

*/
template <typename Edges>
void statistics(Edges &edges) {
  std::sort(edges.begin(), edges.end(), [](const auto& l, const auto& r) {
    return std::tie(std::get<0>(l), std::get<1>(l)) < std::tie(std::get<0>(r), std::get<1>(r));
  });
  std::cerr << "done sorting\n";
  std::tuple<uint32_t, uint32_t> last_edge = std::tie(std::get<0>(edges[0]), std::get<1>(edges[0]));
  std::map<uint32_t, uint32_t> counts_histogram;
  uint32_t repeat_count = 0;
  uint64_t num_edges = 0;
  uint64_t count = 0;
  for (auto& edge : edges) {
    if (++count % 10000000 == 0) {
      std::cerr << "done " << count << " out of " << edges.size() << "\n";
    }
    if (std::tie(std::get<0>(edge), std::get<1>(edge)) == last_edge) {
      repeat_count+=1;
    } else {
      counts_histogram[repeat_count]++;
      repeat_count = 1;
      num_edges++;
      last_edge = std::tie(std::get<0>(edge), std::get<1>(edge));
    }
  }
  counts_histogram[repeat_count]++;
  num_edges++;


  // uint64_t num_updates = 0;
  // absl::flat_hash_map<std::pair<uint32_t, uint32_t>, uint32_t, pair_hash> counts;
  // uint64_t count = 0;
  // for (auto &edge : edges) {
  //   if (++count % 1000000 == 0) {
  //     std::cerr << "done " << count << " out of " << edges.size() << "\n";
  //   }
  //   if (std::get<2>(edge)) {
  //     counts[{std::get<0>(edge), std::get<1>(edge)}]++;
  //     num_updates++;
  //   }
  //   if (!ignore_remove) {
  //     if (!std::get<2>(edge)) {
  //       counts[{std::get<0>(edge), std::get<1>(edge)}]--;
  //       num_updates++;
  //     }
  //   }
  // }
  printf(" num updates = %zu, ", edges.size());
  printf("num edges = %zu\n", num_edges);
  // uint64_t count2 = 0;
  // std::map<uint32_t, uint32_t> counts_histogram;
  // for (auto count : counts) {
  //   if (++count2 % 1000000 == 0) {
  //     std::cerr << "done " << count2 << " out of " << counts.size() << "\n";
  //   }
  //   counts_histogram[std::get<1>(count)]++;
  // }
  for (auto hist : counts_histogram) {
    std::cout << std::get<0>(hist) << ", " << std::get<1>(hist) << "\n";
  }
  return;
}

int main(int32_t argc, char *argv[]) {
  if (argc < 7) {
    std::cout << "must specify all 6 arguments\n";
    return 1;
  }
  auto fname = argv[1];
  printf("graph: %s, ", fname);
  auto src_col = atoll(argv[2]);
  auto dest_col = atoll(argv[3]);
  auto time_col = atoll(argv[4]);
  auto total_cols = atoll(argv[5]);
  bool convert = atoll(argv[6]);
  bool binary = false;
  bool removals = false;
  if (argc >= 8) {
    binary = atoll(argv[7]);
  }
  if (argc >= 9) {
    removals = atoll(argv[8]);
  }
  bool ignore_remove = true;
  if (argc >= 10) {
    ignore_remove = atoll(argv[9]);
  }
  std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> edges;
  if (!binary) {
    edges =
      get_edges(fname, src_col, dest_col, time_col, total_cols, convert);
  } else {
    if (!removals) {
      edges = get_binary_edges_from_file_edges(fname, false);
    } else {
      auto edges_r = get_binary_edges_from_file_edges_with_remove(fname, false);
      std::cerr << "done reading in the graph\n";
      statistics(edges_r);
      return 0;
    }
  }

  std::cerr << "done reading in the graph\n";
  statistics(edges);
  return 0;
}
