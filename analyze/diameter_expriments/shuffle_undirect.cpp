#include "absl/container/flat_hash_map.h"
#include "parlay/utilities.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/random.h>
#include <parlay/sequence.h>
#include <sstream>
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
struct edge {
  uint32_t u;
  uint32_t v;
  uint32_t info;
};
parlay::sequence<edge> parse_input(const std::string input_file) {
  std::cout << "Reading file: " << input_file << "\n";
  auto str = parlay::file_map(input_file);
  auto tokens = parlay::tokens(str, [](char c) { return c == '\n'; });
  return parlay::tabulate(tokens.size(), [&](auto i) {
    auto token = tokens[i];
    std::string line(token.begin(), token.end());
    uint32_t u, v, info;
    std::istringstream iss(line);
    if (!(iss >> u >> v >> info)) {
      throw std::runtime_error("Malformed input line: " + line);
    }
    return edge{u, v, info};
  });
}
int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>\n";
    return 1;
  }
  auto _E = parse_input(argv[1]);
  auto E = parlay::random_shuffle(_E);
  std::ofstream fout(argv[2]);
  absl::flat_hash_map<std::pair<uint32_t, uint32_t>, uint32_t, PairHash, PairEq>
      edge_occurrence;
  for (auto &e : E) {
    auto key = std::pair<uint32_t, uint32_t>(e.u, e.v);
    ++edge_occurrence[key];
    fout << e.u << " " << e.v << " " << (edge_occurrence[key] % 2) << std::endl;
  }
  fout.close();
  return 0;
}
