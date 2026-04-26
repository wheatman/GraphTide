#include "absl/container/flat_hash_map.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <parlay/internal/group_by.h>
#include <parlay/primitives.h>
#include <string>
#include <utility>
#include <vector>
struct txn_t {
  uint32_t u_id;
  uint32_t v_id;
  uint32_t timestamp;
};
struct graph_header {
  uint64_t num_vertices;
  uint64_t num_unique_edges;
  uint64_t total_updates;
  uint8_t if_signal_graph;
  uint8_t vertex_id_type;
  uint8_t timestamp_type;
  uint8_t if_directed;
  uint8_t weight_type;
};
std::string read_from_path(const std::string &filename) {
  std::ifstream fin(filename);
  if (!fin.is_open()) {
    std::cout << "Error: failed to open " << filename << "\n";
    std::abort();
  }
  fin.seekg(0, std::ios::end); // Go to end of file
  size_t length = fin.tellg(); // Get file size
  fin.seekg(0, std::ios::beg); // Go back to beginning

  if (length > 0) {
    std::string data;
    data.resize(length);
    fin.read(&data[0], length);
    return data;
  } else {
    return "";
  }
  fin.close();
}
uint32_t id_of(absl::flat_hash_map<std::string, uint32_t> &map,
               std::vector<std::string> &ids, const std::string &key,
               uint32_t &next_id) {
  auto [it, inserted] = map.try_emplace(key, next_id);
  if (inserted) {
    ids.push_back(key);
    ++next_id;
  }
  return it->second;
}
int main(int argc, char *argv[]) {

  auto txn_time_s = read_from_path("ethereum_blocktime.txt");
  auto separators =
      parlay::pack(parlay::delayed_tabulate(txn_time_s.length(),
                                            [&](size_t i) { return i; }),
                   parlay::delayed_tabulate(txn_time_s.length(), [&](size_t i) {
                     return txn_time_s[i] == '\n';
                   }));
  auto block_time = parlay::tabulate(separators.size(), [&](size_t i) {
    size_t st = (i == 0) ? 0 : separators[i - 1] + 1;
    size_t ed = separators[i];
    auto s = txn_time_s.substr(st, ed - st);
    std::size_t pos = s.find(' ');
    if (pos == std::string::npos) {
      std::cout << "No space found" << std::endl;
      std::abort();
    }
    std::string part1 = s.substr(0, pos);
    std::string part2 = s.substr(pos + 1);
    uint32_t a = std::stoull(part1);
    uint32_t b = std::stoull(part2, nullptr, 16);
    if ((uint64_t)a != i) {
      std::cout << "block id doesn't match" << std::endl;
      std::abort();
    }
    return b;
  });

  std::ifstream f_txns("ethereum_txs.txt");

  absl::flat_hash_map<std::string, uint32_t> user_mapping;
  user_mapping.reserve(100000);
  std::vector<std::string> user_id;
  std::vector<txn_t> txns;
  std::vector<std::pair<uint32_t, uint32_t>> edges;
  user_id.reserve(781489511);
  txns.reserve(781489511);
  edges.reserve(781489511);
  uint32_t num_user = 0;
  std::string coin_type, block_id, txn_id, sender, receiver, weight;
  while (!f_txns.eof()) {
    f_txns >> coin_type >> block_id >> txn_id >> sender >> receiver >> weight;
    auto u_id = id_of(user_mapping, user_id, sender, num_user);
    auto v_id = id_of(user_mapping, user_id, receiver, num_user);
    auto t = block_time[std::stoull(block_id)];
    txn_t txn{u_id, v_id, t};
    txns.emplace_back(txn);
    edges.emplace_back(std::pair(u_id, v_id));
  }

  auto unique_txn = parlay::remove_duplicates(edges);
  graph_header header{num_user,    unique_txn.size(), txns.size(), (uint8_t)1,
                      (uint8_t)12, (uint8_t)12,       (uint8_t)1,  (uint8_t)0};
  std::ofstream ofs("ethereum.bin");
  ofs.write(reinterpret_cast<const char *>(&header), sizeof(header));
  for (auto it : txns)
    ofs.write(reinterpret_cast<const char *>(&it), sizeof(it));
  ofs.close();
  std::ofstream mapping("ethereum.mapping");
  for (auto it : user_id)
    mapping << it << '\n';
  mapping.close();
  return 0;
}