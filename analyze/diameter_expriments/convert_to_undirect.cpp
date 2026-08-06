#include "../../IO/IO.hpp"
// #include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/hash/hash.h"
#include "parlay/utilities.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <parlay/primitives.h>
#include <string>
#include <sys/types.h>
#include <utility>

struct PairHash {
  size_t operator()(const std::pair<uint32_t, uint32_t> &p) const noexcept {
    uint64_t key = (static_cast<uint64_t>(p.first) << 32) | p.second;
    return parlay::hash64(key);
  }
};

struct PairEq {
  bool operator()(const std::pair<uint32_t, uint32_t> &a,
                  const std::pair<uint32_t, uint32_t> &b) const noexcept {
    return a.first == b.first && a.second == b.second;
  }
};

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>\n";
    return 1;
  }

  const std::string input_file = argv[1];
  std::cout << "Reading file: " << input_file << "\n";
  IO::InputGraph graph(input_file);
  std::ofstream fout(argv[2]);

  absl::flat_hash_set<std::pair<uint32_t, uint32_t>, PairHash, PairEq>
      edge_present;

  for (uint64_t i = 0; i < graph.total_updates(); ++i) {
    uint32_t u = graph.get_row_src(i);
    uint32_t v = graph.get_row_dest(i);
    uint32_t info = update_info_to_uint(graph.get_row_info(i));
    if (u == v)
      continue;
    if (u > v)
      std::swap(u, v);
    auto key = std::pair<uint32_t, uint32_t>(u, v);
    if (info == 1) {
      auto [it, inserted] = edge_present.emplace(key);
      if (inserted)
        fout << key.first << ' ' << key.second << ' ' << int(info) << '\n';
    } else { // delete
      auto it = edge_present.find(key);
      if (it != edge_present.end()) {
        edge_present.erase(it); // or it->second = false if you prefer
        fout << key.first << ' ' << key.second << ' ' << int(info) << '\n';
      }
    }
  }
  // for (uint64_t i = 0; i < graph.total_updates(); i++) {

  //   auto it = edge_present.find({u, v});
  //   if (it != edge_present.end()) {
  //     if (info == 1) {
  //       edge_present[{u, v}] = true;
  //       fout << u << " " << v << " " << info << std::endl;
  //     } else {
  //       // info == 0
  //       // deleting a non exist edge, drop the update
  //     }
  //   } else {
  //     if (edge_present[{u, v}] == true &&
  //         info == 0) { // edge in the graph and the update type is delete
  //       edge_present[{u, v}] = false; // mark edge as non exist
  //       fout << u << " " << v << " " << info
  //            << std::endl; // preserve the update
  //     } else if (edge_present[{u, v}] == false && info == 1) {
  //       // edge not in the graph and the udpate type is insert
  //       edge_present[{u, v}] = true;
  //       fout << u << " " << v << " " << info << std::endl;
  //     } else {
  //       // either deleting a non exist edge or inserting an edge that has
  //       // already been in the graph
  //     }
  //   }
  // }
  fout.close();
  return 0;
}
