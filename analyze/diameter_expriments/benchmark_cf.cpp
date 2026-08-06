// This is an example demonstrating dynamic connectivity analysis on a
// dynamic graph undergoing edge insertions and deletions.
//
// If you know the total number of vertices is n and the vertex IDs are
// guaranteed to be within the range [0, n-1], you can declare:
//     SCCWN F(n);
// Otherwise, please use:
//     DyCWN F;
//
// The former is faster than the latter, as the latter relies on a hash map
// to store vertices.
//
// Both structures support the insertion and deletion of edges whose endpoints
// are of type uint32_t. The insert/delete functions return true if the graph's
// connectivity changes after the update, and false otherwise:
//     bool merged = F.insert(u, v);
//     bool split = F.delete(u, v);
//
// To list the size of each connected component, please refer to the function
// print_CC_stat().

#include <algorithm>
#include <cstdint>
#include <dycon/localTree/SCCWN.hpp>
#include <parlay/internal/get_time.h>
#include <parlay/io.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <utility>
#include <vector>
struct temporal_edge {
  uint32_t u;
  uint32_t v;
  uint32_t info;
};
parlay::sequence<temporal_edge> parse_input(const std::string input_file) {
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
    return temporal_edge{u, v, info};
  });
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <input_file> \n";
    return 1;
  }
  auto E = parse_input(argv[1]);
  auto V = parlay::map(E, [&](temporal_edge &e) { return std::max(e.u, e.v); });
  auto n = parlay::reduce(V, parlay::maximum<uint32_t>()) + 1;
  std::cout << n << " " << E.size() << std::endl;
  SCCWN F(n);
  parlay::internal::timer t_updates;
  for (auto e : E) {
    if (e.info == 1)
      F.insert(e.u, e.v);
    else
      F.remove(e.u, e.v);
  }
  t_updates.next("updates");
  return 0;
}