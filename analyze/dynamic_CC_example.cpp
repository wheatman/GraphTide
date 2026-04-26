// This is an example demonstrating dynamic connectivity analysis on a
// dynamic graph undergoing edge insertions and deletions.
//
// The header file to you need to use is
//     #include "dycon/SCCWN.hpp"
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
// are of type uint32_t. The insert and remove functions return true if the
// graph's connectivity changes after the update, and false otherwise:
//     bool merged = F.insert(u, v);
//     bool split = F.delete(u, v);
//
// To list the size of each connected component, please refer to the function
// print_CC_stat().
#include "localTree/SCCWN.hpp"
#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>
using edge_t = std::vector<std::pair<uint32_t, uint32_t>>;

template <typename T> void print_CC_stat(T &F) {
  // list the size of each connect component
  std::cout << "Connected Component Statistics" << std::endl;
  auto CCs = F.CC_stat();
  std::cout << "number of CCs: " << CCs.size() << std::endl;
  for (auto &it : CCs)
    std::cout << "size of CC: " << it.second << std::endl;
}
void fixed_set_vertices(edge_t &insertions, edge_t &deletions, uint32_t n) {
  SCCWN F(n);
  for (auto &it : insertions) {
    // bool merged = F.insert(it.first, it.second);
    F.insert(it.first, it.second);
    print_CC_stat(F);
  }
  for (auto &it : deletions) {
    // bool split = F.remove(it.first, it.second);
    F.remove(it.first, it.second);
    print_CC_stat(F);
  }
}
void arbitrary_vertices(edge_t &insertions, edge_t &deletions) {
  DyCWN F;
  for (auto &it : insertions) {
    // bool merged = F.insert(it.first, it.second);
    F.insert(it.first, it.second);
    print_CC_stat(F);
  }
  for (auto &it : deletions) {
    // bool split = F.remove(it.first, it.second);
    F.remove(it.first, it.second);
    print_CC_stat(F);
  }
}
int main() {
  // construct example graph
  uint32_t n = 20;
  uint32_t m = n - 1;
  std::vector<std::pair<uint32_t, uint32_t>> insertions(m);
  std::vector<std::pair<uint32_t, uint32_t>> deletions(m);
  for (auto i = 0; i < m; i++)
    insertions[i] = deletions[i] = std::pair(i, i + 1);
  std::random_shuffle(deletions.begin(), deletions.end());

  // example 1
  // when you know the vertices id will be 0...n-1
  std::cout << "Running on fixed set of vertices" << std::endl;
  fixed_set_vertices(insertions, deletions, n);

  std::cout << "----------------------------------------------------"
            << std::endl;
  // example 2
  // when you don't know the total number or range of vertices
  std::cout << "Running on arbitrary vertices" << std::endl;
  arbitrary_vertices(insertions, deletions);
  return 0;
}