#include <dynamic_graph/dynamic_connectivity.hpp>
#include <parlay/internal/get_time.h>
#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>
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
  DynamicConnectivity HDT(n);
  parlay::internal::timer t_updates;
  for (auto e : E) {
    if (e.info == 1)
      HDT.AddEdge(UndirectedEdge(e.u, e.v));
    else
      HDT.DeleteEdge(UndirectedEdge(e.u, e.v));
  }
  t_updates.next("updates");
  return 0;
}
