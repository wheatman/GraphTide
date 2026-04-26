#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "parlay/sequence.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <parlay/internal/group_by.h>
#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/slice.h>
#include <string>
#include <utility>
std::string read_from_path(const std::string &filename) {
  std::ifstream fin(filename);
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
std::vector<std::string> splitByComma(const std::string &input) {
  std::vector<std::string> result;
  std::stringstream ss(input);
  std::string token;
  while (std::getline(ss, token, ',')) {
    result.push_back(token);
  }
  return result;
}
uint32_t seconds_diff(const std::string &t1, const std::string &t2) {
  std::tm tm1 = {}, tm2 = {};
  std::istringstream ss1(t1);
  std::istringstream ss2(t2);

  ss1 >> std::get_time(&tm1, "%Y-%m-%d %H:%M:%S");
  ss2 >> std::get_time(&tm2, "%Y-%m-%d %H:%M:%S");

  std::time_t time1 = std::mktime(&tm1);
  std::time_t time2 = std::mktime(&tm2);

  return std::difftime(time2, time1);
}
uint32_t get_time_in_sec(std::string &t1,
                         std::string t2 = "2018-12-31 00:00:00") {
  std::string epoch_time = "2018-12-31 00:00:00";
  std::string h = t1.substr(0, epoch_time.length());
  std::string l = t2.substr(0, epoch_time.length());
  // std::string current_time = s.substr(0, s.find('+'));

  return seconds_diff(l, h);
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
// struct flight {
//   std::string src;
//   std::string dest;
//   bool operator==(const flight &other) const {
//     return src == other.src && dest == other.dest;
//   }
// };
// using flight_t = std::pair<std::string, std::string>;
struct flight {
  uint32_t src;
  uint32_t dest;
  uint32_t timestamp;
  uint32_t duration;
};
struct flight_unweight {
  uint32_t src;
  uint32_t dest;
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
int main() {
  std::string filename = "flight_opensky.csv";
  std::string s = read_from_path(filename);
  auto separators = parlay::pack(
      parlay::delayed_tabulate(s.length(), [&](size_t i) { return i; }),
      parlay::delayed_tabulate(s.length(),
                               [&](size_t i) { return s[i] == '\n'; }));
  auto tables = parlay::delayed_tabulate(separators.size(), [&](size_t i) {
    size_t st = (i == 0) ? 0 : separators[i - 1] + 1;
    size_t ed = separators[i];
    return splitByComma(s.substr(st, ed - st));
    // return s.substr(st, ed - st);
  });
  std::cout << tables.size() << std::endl;
  parlay::sequence<parlay::sequence<std::pair<uint32_t, uint32_t>>>
      valid_flight(160000000);
  absl::flat_hash_map<std::string, uint32_t> airports_mapping;
  absl::flat_hash_set<uint64_t> edges;
  uint32_t num_airports = 0;
  uint32_t num_total_updates = 0;
  std::vector<std::string> airports_id;
  for (auto i = 1; i < tables.size(); i++) {
    if (tables[i][0] == "" || tables[i][1] == "" ||
        (tables[i][0] == tables[i][1])) {
      continue;
    } else {
      uint32_t a =
          id_of(airports_mapping, airports_id, tables[i][0], num_airports);
      uint32_t b =
          id_of(airports_mapping, airports_id, tables[i][1], num_airports);
      uint32_t t = get_time_in_sec(tables[i][2]);
      edges.insert((uint64_t(b) << 32) | a);
      valid_flight[t].emplace_back(std::pair(a, b));
      num_total_updates++;
    }
  }
  graph_header header{airports_id.size(), edges.size(), num_total_updates,
                      (uint8_t)1,         (uint8_t)12,  (uint8_t)12,
                      (uint8_t)1,         (uint8_t)0};
  std::ofstream ofs("flight_opensky.bin");
  ofs.write(reinterpret_cast<const char *>(&header), sizeof(header));
  for (auto i = 0; i < valid_flight.size(); i++) {
    for (auto j = 0; j < valid_flight[i].size(); j++) {
      ofs.write(reinterpret_cast<const char *>(&valid_flight[i][j].first),
                sizeof(uint32_t));
      ofs.write(reinterpret_cast<const char *>(&valid_flight[i][j].second),
                sizeof(uint32_t));
      ofs.write(reinterpret_cast<const char *>(&i), sizeof(uint32_t));
    }
  }
  ofs.close();
  std::ofstream mapping("flight_opensky.mapping");
  for (auto it : airports_id)
    mapping << it << '\n';
  mapping.close();

  return 0;
}