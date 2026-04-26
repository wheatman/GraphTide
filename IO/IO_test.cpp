#include "IO.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <graph_file1> [<graph_file2> ...]\n";
    return 1;
  }

  for (int i = 1; i < argc; ++i) {
    const std::string filename = argv[i];
    std::cout << "Reading file: " << filename << "\n";

    IO::InputGraph graph(filename);

    graph.print_summary();
    graph.print_rows();

    std::cout << "----------------------------\n";
  }

  return 0;
}
