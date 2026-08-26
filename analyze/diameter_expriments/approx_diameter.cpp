#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "union_find.h"

#include <parlay/io.h>
#include <parlay/parallel.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>

using vertex = uint32_t;
using Graph = std::vector<std::vector<vertex>>;

// ============================================================
// Atomic minimum
// ============================================================

static inline void atomic_min(std::atomic<int64_t> &x, int64_t value) {

  int64_t old = x.load(std::memory_order_relaxed);

  while (old == -1 || value < old) {

    if (x.compare_exchange_weak(old, value, std::memory_order_relaxed,
                                std::memory_order_relaxed)) {
      return;
    }
  }
}

// ============================================================
// Find one vertex in the largest connected component
//
// Uses ParlayLib's union_find.
//
// We always link:
//
//     larger root -> smaller root
//
// so concurrent links cannot form cycles.
// ============================================================

vertex largest_component_source(uint32_t n, const Graph &G) {

  using uf_vertex = int64_t;

  union_find<uf_vertex> uf(n);

  // proposal[r] = root that r should link to.
  // -1 means no proposal.
  auto proposal = parlay::tabulate<std::atomic<uf_vertex>>(
      n, [](size_t) { return uf_vertex{-1}; });

  // ==========================================================
  // Parallel hooking rounds
  // ==========================================================

  while (true) {

    // Reset proposals.
    parlay::parallel_for(0, static_cast<size_t>(n), [&](size_t i) {
      proposal[i].store(-1, std::memory_order_relaxed);
    });

    // --------------------------------------------------------
    // Examine each undirected edge once.
    //
    // Since each edge appears twice in G, process only u < v.
    // --------------------------------------------------------

    parlay::parallel_for(0, static_cast<size_t>(n), [&](size_t ui) {
      vertex u = static_cast<vertex>(ui);

      const auto &neighbors = G[u];

      parlay::parallel_for(
          0, neighbors.size(),
          [&](size_t j) {
            vertex v = neighbors[j];

            // Ignore reverse edge and self-loop.
            if (u >= v) {
              return;
            }

            uf_vertex ru = uf.find(static_cast<uf_vertex>(u));

            uf_vertex rv = uf.find(static_cast<uf_vertex>(v));

            if (ru == rv) {
              return;
            }

            // Always larger root -> smaller root.
            uf_vertex hi = std::max(ru, rv);

            uf_vertex lo = std::min(ru, rv);

            // If multiple edges propose a target for hi,
            // keep the smallest target.
            atomic_min(proposal[hi], lo);
          },
          2048);
    });

    // --------------------------------------------------------
    // Check whether there are links to perform.
    // --------------------------------------------------------

    bool changed = parlay::any_of(proposal, [](const auto &p) {
      return p.load(std::memory_order_relaxed) != -1;
    });

    if (!changed) {
      break;
    }

    // --------------------------------------------------------
    // Perform links.
    // --------------------------------------------------------

    parlay::parallel_for(0, static_cast<size_t>(n), [&](size_t i) {
      uf_vertex target = proposal[i].load(std::memory_order_relaxed);

      if (target != -1) {

        uf.link(static_cast<uf_vertex>(i), target);
      }
    });
  }

  // ==========================================================
  // Find final root of every vertex
  // ==========================================================

  auto roots = parlay::tabulate<vertex>(n, [&](size_t i) {
    return static_cast<vertex>(uf.find(static_cast<uf_vertex>(i)));
  });

  // ==========================================================
  // Count component sizes
  // ==========================================================

  auto component_sizes =
      parlay::histogram_by_index(roots, static_cast<size_t>(n));

  // ==========================================================
  // Find largest connected component
  //
  // Its root is itself a vertex in the component, so it can
  // directly be used as the BFS source.
  // ==========================================================

  auto largest_it = parlay::max_element(component_sizes);

  vertex largest_root =
      static_cast<vertex>(largest_it - component_sizes.begin());

  return largest_root;
}

// ============================================================
// Single-sweep parallel BFS
//
// Adapted from ParlayLib examples/BFS.h.
//
// Instead of returning all BFS frontiers, we only count BFS
// levels.
//
// Return:
//
//     eccentricity(start)
//
// Therefore:
//
//     D / 2 <= returned value <= D
//
// where D is the diameter of the component containing start.
// ============================================================

uint32_t approximate_diameter_BFS(vertex start, const Graph &G) {

  // ----------------------------------------------------------
  // Atomic visited array
  // ----------------------------------------------------------

  auto visited = parlay::tabulate<std::atomic<bool>>(
      G.size(), [&](size_t i) { return i == start; });

  // ----------------------------------------------------------
  // Initial frontier
  // ----------------------------------------------------------

  parlay::sequence<vertex> frontier(1, start);

  // Source is at distance 0.
  uint32_t diameter = 0;

  // ==========================================================
  // Level-synchronous BFS
  // ==========================================================

  while (!frontier.empty()) {

    // --------------------------------------------------------
    // Gather all neighbors of the current frontier.
    //
    // Same structure as ParlayLib BFS:
    //
    //     flatten(map(frontier, ...))
    // --------------------------------------------------------

    auto out =
        parlay::flatten(parlay::map(frontier, [&](vertex u) { return G[u]; }));

    // --------------------------------------------------------
    // Keep only newly visited vertices.
    //
    // If multiple edges point to the same vertex, exactly one
    // compare_exchange succeeds.
    // --------------------------------------------------------

    auto next_frontier = parlay::filter(out, [&](vertex v) {
      bool expected = false;

      return visited[v].compare_exchange_strong(
          expected, true, std::memory_order_relaxed, std::memory_order_relaxed);
    });

    // --------------------------------------------------------
    // No new vertices:
    //
    // current frontier is the final BFS level.
    // --------------------------------------------------------

    if (next_frontier.empty()) {
      break;
    }

    ++diameter;

    frontier = std::move(next_frontier);
  }

  return diameter;
}

// ============================================================
// Approximate diameter of the largest connected component
// ============================================================

uint32_t approximate_lcc_diameter(uint32_t n, const Graph &G) {

  if (n == 0) {
    return 0;
  }

  // Find a vertex belonging to the LCC.
  vertex source = largest_component_source(n, G);

  // One BFS from that source.
  return approximate_diameter_BFS(source, G);
}

// ============================================================
// Temporal edge
//
// info == 1 : insertion
// otherwise : deletion
// ============================================================

struct temporal_edge {
  uint32_t u;
  uint32_t v;
  uint32_t info;
};

// ============================================================
// Parse input
// ============================================================

parlay::sequence<temporal_edge> parse_input(const std::string &input_file) {

  auto str = parlay::file_map(input_file);

  auto tokens = parlay::tokens(str, [](char c) { return c == '\n'; });

  return parlay::tabulate(tokens.size(), [&](size_t i) {
    auto token = tokens[i];

    std::string line(token.begin(), token.end());

    uint32_t u;
    uint32_t v;
    uint32_t info;

    std::istringstream iss(line);

    if (!(iss >> u >> v >> info)) {

      throw std::runtime_error("Malformed input line: " + line);
    }

    return temporal_edge{u, v, info};
  });
}

// ============================================================
// Apply buffered updates to one adjacency list
//
// The adjacency list stays sorted.
//
// If the same edge receives multiple updates in one batch,
// the last event wins.
// ============================================================

std::vector<uint32_t>
update(const std::vector<uint32_t> &nghs,
       std::vector<std::pair<uint32_t, uint32_t>> &events) {

  if (events.empty()) {
    return nghs;
  }

  // Stable sort preserves temporal order among events involving
  // the same neighbor.
  std::stable_sort(
      events.begin(), events.end(),
      [](const auto &a, const auto &b) { return a.first < b.first; });

  size_t p = 0;
  size_t q = 0;

  std::vector<uint32_t> res;

  res.reserve(nghs.size() + events.size());

  // ==========================================================
  // Merge existing adjacency list and update events
  // ==========================================================

  while (p < nghs.size() && q < events.size()) {

    uint32_t event_vertex = events[q].first;

    size_t q_end = q + 1;

    // Find all events for this same neighbor.
    while (q_end < events.size() && events[q_end].first == event_vertex) {

      ++q_end;
    }

    // Last event wins.
    bool keep_event = events[q_end - 1].second;

    if (nghs[p] < event_vertex) {

      res.push_back(nghs[p]);

      ++p;

    } else if (nghs[p] == event_vertex) {

      if (keep_event) {

        res.push_back(nghs[p]);
      }

      ++p;
      q = q_end;

    } else {

      if (keep_event) {

        res.push_back(event_vertex);
      }

      q = q_end;
    }
  }

  // ----------------------------------------------------------
  // Remaining existing neighbors
  // ----------------------------------------------------------

  while (p < nghs.size()) {

    res.push_back(nghs[p]);

    ++p;
  }

  // ----------------------------------------------------------
  // Remaining events
  // ----------------------------------------------------------

  while (q < events.size()) {

    uint32_t event_vertex = events[q].first;

    size_t q_end = q + 1;

    while (q_end < events.size() && events[q_end].first == event_vertex) {

      ++q_end;
    }

    if (events[q_end - 1].second) {

      res.push_back(event_vertex);
    }

    q = q_end;
  }

  events.clear();

  return res;
}

// ============================================================
// Apply all buffered updates
//
// Important:
// vertices with no updates in this batch are skipped completely.
// ============================================================

void apply_buffered_updates(
    Graph &G, std::vector<std::vector<std::pair<uint32_t, uint32_t>>> &buffer) {

  parlay::parallel_for(0, G.size(), [&](size_t v) {
    if (!buffer[v].empty()) {

      G[v] = update(G[v], buffer[v]);
    }
  });
}

// ============================================================
// Main
//
// argv[1] = input file
// argv[2] = number of diameter samples
//
// Let:
//
//     N = number of updates
//     S = num_samples
//
// batch_size = floor(N / S)
//
// The first S - 1 samples process exactly batch_size updates.
//
// The last sample processes every remaining update.
//
// Therefore:
//     - diameter is computed exactly num_samples times
//     - every update is applied
// ============================================================

int main(int argc, char *argv[]) {

  if (argc < 3) {
    return 1;
  }

  // ==========================================================
  // Read update sequence
  // ==========================================================

  auto E = parse_input(argv[1]);

  size_t num_samples = std::stoull(argv[2]);

  // ==========================================================
  // Determine number of vertices
  // ==========================================================

  auto V = parlay::map(
      E, [&](const temporal_edge &e) { return std::max(e.u, e.v); });

  uint32_t n = parlay::reduce(V, parlay::maximum<uint32_t>()) + 1;

  // ==========================================================
  // Fixed batch size
  //
  // Assumption:
  //
  //     E.size() >>> num_samples
  //
  // so batch_size is safely > 0.
  // ==========================================================

  size_t batch_size = E.size() / num_samples;

  // ==========================================================
  // Current dynamic graph
  // ==========================================================

  Graph G(n);

  // ==========================================================
  // Buffered updates for every vertex
  //
  // pair:
  //
  //     first  = neighbor
  //     second = 1 insertion / 0 deletion
  // ==========================================================

  std::vector<std::vector<std::pair<uint32_t, uint32_t>>> buffer(n);

  // ==========================================================
  // Current update position
  // ==========================================================

  size_t begin = 0;

  // ==========================================================
  // Compute diameter exactly num_samples times
  // ==========================================================

  for (size_t sample = 0; sample < num_samples; ++sample) {

    // --------------------------------------------------------
    // First num_samples - 1 batches have exactly batch_size
    // updates.
    //
    // Last batch absorbs the remainder.
    // --------------------------------------------------------

    size_t end = (sample + 1 == num_samples) ? E.size() : begin + batch_size;

    // --------------------------------------------------------
    // Buffer updates in [begin, end)
    // --------------------------------------------------------

    for (size_t i = begin; i < end; ++i) {

      const auto &e = E[i];

      if (e.info == 1) {

        // Insert edge u -> v
        buffer[e.u].emplace_back(e.v, 1);

        // Insert edge v -> u
        buffer[e.v].emplace_back(e.u, 1);

      } else {

        // Delete edge u -> v
        buffer[e.u].emplace_back(e.v, 0);

        // Delete edge v -> u
        buffer[e.v].emplace_back(e.u, 0);
      }
    }

    // --------------------------------------------------------
    // Apply current batch
    // --------------------------------------------------------

    apply_buffered_updates(G, buffer);

    // --------------------------------------------------------
    // Recompute LCC and run one BFS
    // --------------------------------------------------------

    uint32_t diameter = approximate_lcc_diameter(n, G);

    // --------------------------------------------------------
    // One output for each sample
    // --------------------------------------------------------

    std::cout << diameter << '\n';

    begin = end;
  }

  return 0;
}