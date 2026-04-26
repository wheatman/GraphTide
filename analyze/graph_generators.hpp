#pragma once

#include <algorithm>
#include <cstdint>
#include <random>
#include <tuple>
#include <vector>

#include "parallel.h"
#include "rmat_util.h"

// generate an rmat graph not symeterized
template <class node_t = uint32_t, class timestamp_t = uint32_t>
std::vector<std::tuple<node_t, node_t, timestamp_t>>
generate_rmat(node_t num_nodes, uint64_t num_edges, double a = .5,
              double b = .1, double c = .1) {
  std::vector<std::tuple<node_t, node_t, timestamp_t>> edges;
  std::random_device rd;
  auto r = rmat_util::random_aspen(rd());
  uint64_t nn = 1UL << (rmat_util::log2_up(num_nodes) - 1);
  auto rmat = rmat_util::rMat<uint32_t>(nn, r.ith_rand(0), a, b, c);
  for (uint64_t i = 0; i < num_edges; i++) {
    std::pair<uint32_t, uint32_t> edge = rmat(i);
    edges.emplace_back(edge.first, edge.second, i);
  }
  return edges;
}

// generate a ER graph, not symeterized
template <class node_t = uint32_t, class timestamp_t = uint32_t>
std::vector<std::tuple<node_t, node_t, timestamp_t>>
generate_er(node_t num_nodes, float p) {
  std::vector<std::vector<std::tuple<node_t, node_t, timestamp_t>>>
      edge_buckets(getWorkers());
  p_for_1(int k = 0; k < getWorkers(); k++) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    std::uniform_int_distribution<uint64_t> dis_int(
        0, static_cast<uint64_t>(num_nodes * p) * num_nodes);
    std::vector<std::tuple<node_t, node_t, timestamp_t>> bucket;
    uint64_t start = k * (num_nodes / getWorkers());
    uint64_t end = (k + 1) * (num_nodes / getWorkers());
    if (k + 1 == getWorkers()) {
      end = num_nodes;
    }
    for (uint64_t i = start; i < end; i++) {
      for (uint64_t j = 0; j < num_nodes; j++) {
        if (dis(g) <= p) {
          bucket.emplace_back(i, j, dis_int(g));
        }
      }
    }
    edge_buckets[k] = bucket;
  }
  std::vector<size_t> edge_counts(getWorkers() + 1);
  for (int i = 0; i < getWorkers(); i++) {
    edge_counts[i + 1] = edge_counts[i] + edge_buckets[i].size();
  }
  std::vector<std::tuple<node_t, node_t, timestamp_t>> edges(
      edge_counts[getWorkers()]);
  for (int i = 0; i < getWorkers(); i++) {
    for (size_t j = 0; j < edge_buckets[i].size(); j++) {
      edges[edge_counts[i] + j] = edge_buckets[i][j];
    }
  }
  std::sort(edges.begin(), edges.end(), [](auto const &t1, auto const &t2) {
    return std::get<2>(t1) < std::get<2>(t2);
  });

  return edges;
}

template <class node_t = uint32_t, class timestamp_t = uint32_t>
std::vector<std::tuple<node_t, node_t, timestamp_t>>
generate_watts_strogatz(node_t num_nodes, uint64_t K, double beta) {
  std::vector<std::tuple<node_t, node_t, timestamp_t>> edges(num_nodes * K);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> real_dis(0, 1.0);
  std::uniform_int_distribution<uint64_t> int_dist(0, num_nodes - 1);
  std::uniform_int_distribution<uint64_t> dis_time(0, num_nodes * K);
  for (uint64_t i = 0; i < num_nodes; i++) {
    uint64_t start;
    if (i > (K / 2)) {
      start = i - (K / 2);
    } else {
      start = num_nodes - (i - (K / 2));
    }
    uint64_t end = (i + (K / 2)) % num_nodes;
    uint64_t count = 0;
    for (uint64_t j = start; j != end; j = (j + 1) % num_nodes) {
      uint64_t dest = j;
      if (real_dis(gen) < beta) {
        dest = int_dist(gen);
      }
      edges[i * K + count] = {i, dest, dis_time(gen)};
      count++;
    }
  }

  std::sort(edges.begin(), edges.end(), [](auto const &t1, auto const &t2) {
    return std::get<2>(t1) < std::get<2>(t2);
  });
  return edges;
}