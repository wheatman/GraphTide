#include <vector>
#include <queue>
#include <utility>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <memory>
#include "absl/container/flat_hash_set.h"

#ifndef NODE_TYPE
#define NODE_TYPE uint32_t
#endif

#ifndef TIMESTAMP_TYPE
#define TIMESTAMP_TYPE uint32_t
#endif

using DegreePair = std::pair<size_t, NODE_TYPE>;


class EdgeOrientation {
private:
    std::vector<absl::flat_hash_set<NODE_TYPE>> out_edges;
    std::vector<std::queue<NODE_TYPE>> Q;
    std::vector<size_t> out_degree;
    std::priority_queue<DegreePair> degree_heap;
    const size_t n;
    const size_t k;

public:
    EdgeOrientation(size_t n, size_t k) : out_edges(n), Q(n), out_degree(n, 0), n(n), k(k) {}
    size_t insert_edge(NODE_TYPE u, NODE_TYPE v);
    size_t delete_edge(NODE_TYPE u, NODE_TYPE v);

    size_t get_max_out_degree() {
        if (degree_heap.empty()) return 0;
        while (!degree_heap.empty()) {
            size_t max_deg = degree_heap.top().first;
            NODE_TYPE v = degree_heap.top().second;
            if (max_deg == out_degree[v] && max_deg > 0)
                return max_deg;
            else degree_heap.pop();
        }
        return 0;
    }

private:
    void check_bounds(NODE_TYPE u) {
        if (u >= n) {
            std::cerr << "Vertex id " << u << " passed out of bounds (N=" << n << ")." << std::endl;
            std::abort();
        }
    }

    void update_degree(NODE_TYPE v, int delta) {
        if (delta > 0) {
            out_degree[v] += (size_t)delta;
        } else {
            if (out_degree[v] >= (size_t)(-delta)) {
                 out_degree[v] -= (size_t)(-delta);
            } else {
                out_degree[v] = 0;
            }
        }
        if (out_degree[v] > 0) {
            degree_heap.push({out_degree[v], v});
        }
    }

    size_t k_flips(size_t k);
};

size_t EdgeOrientation::k_flips(size_t k) {
    size_t flip_count = 0;

    for (size_t i = 0; i < k; ++i) {
        NODE_TYPE v;
        bool found_max = false;
        while (!degree_heap.empty()) {
            size_t max_deg = degree_heap.top().first;
            v = degree_heap.top().second;
            degree_heap.pop();
            if (max_deg == out_degree[v] && max_deg > 0) {
                found_max = true;
                break; 
            }
        }
        if (!found_max) break;

        NODE_TYPE u = Q[v].front();
        Q[v].pop();
        if (!out_edges[v].contains(u)) {
            i--; 
            continue; 
        }

        out_edges[v].erase(u);
        out_edges[u].insert(v);
        Q[u].push(v);
        update_degree(v, -1);
        update_degree(u, 1);
        flip_count++;
    }
    
    return flip_count;
}

size_t EdgeOrientation::insert_edge(NODE_TYPE u, NODE_TYPE v) {
    check_bounds(u);
    check_bounds(v);
    if (out_edges[u].contains(v) || out_edges[v].contains(u)) return 0;
    
    out_edges[u].insert(v);
    Q[u].push(v);
    update_degree(u, 1);
    return k_flips(k);
}

size_t EdgeOrientation::delete_edge(NODE_TYPE u, NODE_TYPE v) {
    check_bounds(u);
    check_bounds(v);
    if (!out_edges[u].contains(v) && !out_edges[v].contains(u)) {
        std::cerr << "Attempted deletion of an edge that doesn't exists." << std::endl;
        return 0; 
    }

    if (out_edges[u].contains(v)) {
        out_edges[u].erase(v);
        update_degree(u, -1);
    } else if (out_edges[v].contains(u)) {
        out_edges[v].erase(u);
        update_degree(v, -1);
    }
    return k_flips(k);
}
