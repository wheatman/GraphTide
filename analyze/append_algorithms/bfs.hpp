#pragma once
#include <string>

// This graph is the underlying graph datastructure
//  for now its just a std::vector<absl::flat_hash_set<node_t>>
template <typename Graph_container> struct append_bfs {
  using node_t = Graph_container::value_type::value_type;

  static constexpr std::string_view header = "max_distance_from_src";

  append_bfs(const Graph_container &g_) : g(g_) {}

  // put something in the destructor to run at the end
  ~append_bfs() {
    // Write histogram to separate file
    std::ofstream hist_file(histogram_filepath);

    for (auto [size, count] : get_bfs_update_sizes()) {
      hist_file << size << ", " << count << "\n";
    }

    hist_file.close();
  }

  template <typename params> void set_params(const params &p) {
    node_t src = p.src;
    bfs_distances.resize(p.total_nodes, std::numeric_limits<uint64_t>::max());
    bfs_distances[src] = 0;
    histogram_filepath = p.bfs_histogram_filepath;
  }

  // this is called the first time an edge is added to this vertex
  void add_vertex(node_t vertex_id) {}

  // this is called the first time this edge is added to the graph
  void add_edge(node_t source, node_t dest) {
    uint64_t update_size = 0;
    if (bfs_distances[source] != std::numeric_limits<uint64_t>::max() &&
        bfs_distances[dest] > bfs_distances[source] + 1) {
      update_size += update_bfs(dest, bfs_distances[source] + 1);
    } else if (bfs_distances[dest] != std::numeric_limits<uint64_t>::max() &&
               bfs_distances[source] > bfs_distances[dest] + 1) {
      update_size += update_bfs(source, bfs_distances[dest] + 1);
    }
    bfs_update_size_histogram[update_size] += 1;
  }

  std::string get_string_values() const {
    return std::to_string(distances_histogram.size());
  }

private:
  const Graph_container &g;
  node_t src;

  std::vector<uint64_t> bfs_distances;
  absl::flat_hash_map<uint64_t, node_t> distances_histogram;
  absl::flat_hash_map<uint64_t, uint64_t> bfs_update_size_histogram;
  std::string histogram_filepath;

  uint64_t update_bfs(node_t src, uint64_t new_distance) {
    uint64_t count_updated = 0;

    {
      if (bfs_distances[src] != std::numeric_limits<uint64_t>::max()) {
        distances_histogram[bfs_distances[src]] -= 1;
      }
      bfs_distances[src] = new_distance;
      distances_histogram[bfs_distances[src]] += 1;
      count_updated += 1;
    }
    std::vector<node_t> frontier = {src};
    while (!frontier.empty()) {
      std::vector<node_t> next_frontier;
      for (node_t s : frontier) {
        for (node_t t : g[s]) {
          if (bfs_distances[t] > bfs_distances[s] + 1) {
            if (bfs_distances[t] != std::numeric_limits<uint64_t>::max()) {
              distances_histogram[bfs_distances[t]] -= 1;
              if (distances_histogram[bfs_distances[t]] == 0) {
                distances_histogram.erase(bfs_distances[t]);
              }
            }
            bfs_distances[t] = bfs_distances[s] + 1;
            distances_histogram[bfs_distances[t]] += 1;
            next_frontier.push_back(t);
            count_updated += 1;
          }
        }
      }
      std::swap(frontier, next_frontier);
    }
    return count_updated;
  }
  std::vector<std::pair<uint64_t, uint64_t>> get_bfs_update_sizes() const {
    std::vector<std::pair<uint64_t, uint64_t>> update_sizes;
    for (auto [size, count] : bfs_update_size_histogram) {
      update_sizes.emplace_back(size, count);
    }
    std::sort(update_sizes.begin(), update_sizes.end());
    return update_sizes;
  }
};
