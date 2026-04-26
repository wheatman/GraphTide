#include <bits/stdc++.h>

#include "parlay/io.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"
using namespace std;

double distance(double lat_a, double lon_a, double lat_b, double lon_b) {
  constexpr double pi = 3.14159265358979323846L;
  constexpr double deg_to_rad = pi / 180.0L;
  constexpr double earth_radius = 6371007.181L;

  if (lat_a == lat_b && lon_a == lon_b) {
    return 0.0L;
  }

  double vec[4] = {
      (lat_b - lat_a) * deg_to_rad,  // Δlat
      (lon_b - lon_a) * deg_to_rad,  // Δlon
      lat_a * deg_to_rad,            // lat1
      lat_b * deg_to_rad             // lat2
  };

  double sin_dlat2 = sinl(0.5L * vec[0]);
  double sin_dlon2 = sinl(0.5L * vec[1]);
  double cos_lat1 = cosl(vec[2]);
  double cos_lat2 = cosl(vec[3]);

  double a =
      sin_dlat2 * sin_dlat2 + cos_lat1 * cos_lat2 * sin_dlon2 * sin_dlon2;

  double c = 2.0L * atan2l(sqrtl(a), sqrtl(1.0L - a));
  return earth_radius * c;
}

int main() {
  parlay::sequence<tuple<long long, double, double>> coords;
  size_t n = 0, m = 0;
  int cnt = 0;
  string line;

  ifstream ifs("nodes.csv");
  getline(ifs, line);
  while (getline(ifs, line)) {
    istringstream iss(line);
    string sid, sx, sy;
    getline(iss, sid, ',');
    getline(iss, sx, ',');
    getline(iss, sy, ',');
    long long id = stoll(sid);
    double x = stod(sx), y = stod(sy);
    if (cnt++ < 5) {
      printf("id: %lld, x: %f, y: %f\n", id, x, y);
    }
    coords.push_back({id, x, y});
  }
  parlay::sort_inplace(parlay::make_slice(coords),
                       [](const tuple<long long, double, double> &a,
                          const tuple<long long, double, double> &b) {
                         return std::get<0>(a) < std::get<0>(b);
                       });

  parlay::chars point_chars;
  point_chars.insert(point_chars.end(),
                     parlay::to_chars("pbbs_sequencePoint2d\n"));
  point_chars.insert(
      point_chars.end(), parlay::flatten(parlay::tabulate(n * 2, [&](size_t i) {
        if (i % 2 == 0) {
          return parlay::to_chars(to_string(get<1>(coords[i / 2])) + ' ' +
                                  to_string(get<2>(coords[i / 2])));
        } else {
          return parlay::to_chars('\n');
        }
      })));
  chars_to_file(point_chars, "coords.pbbs");

  n = coords.size();
  unordered_map<long long, uint32_t> mp;
  for (size_t i = 0; i < coords.size(); i++) {
    mp[std::get<0>(coords[i])] = i;
  }

  vector<vector<pair<int, double>>> G(n);
  ifs = ifstream("edges.csv");
  cnt = 0;
  getline(ifs, line);
  double mn = INT_MAX, mx = 0, avg = 0;
  while (getline(ifs, line)) {
    istringstream iss(line);
    string sid, su, sv, sw;
    getline(iss, sid, ',');
    getline(iss, sid, ',');
    getline(iss, su, ',');
    getline(iss, sv, ',');
    getline(iss, sw, ',');
    long long id_u = stoll(su), id_v = stoll(sv);
    double w = stold(sw);
    assert(mp.find(id_u) != mp.end());
    assert(mp.find(id_v) != mp.end());
    uint32_t u = mp[id_u], v = mp[id_v];
    auto [id1, x1, y1] = coords[u];
    auto [id2, x2, y2] = coords[v];
    double d = distance(y1, x1, y2, x2);
    if (cnt++ < 5) {
      printf("u: %u, v: %u, w: %f, d: %f\n", u, v, w, d);
      fflush(stdout);
    }
    if (d > w) {
      printf("TOO LARGE: d: %.10f, w: %.10f\n", d, w);
    }
    assert(d <= w);
    mx = max(mx, d / w);
    mn = min(mn, d / w);
    if (w != 0) {
      avg += d / w;
    }
    assert(u < n && v < n);
    G[u].push_back({v, w});
    m++;
  }

  vector<size_t> offsets(n + 1);
  vector<uint32_t> edges(m);
  vector<double> weights(m);
  for (size_t i = 0; i < n; i++) {
    sort(begin(G[i]), end(G[i]));
    offsets[i + 1] = offsets[i] + G[i].size();
    for (size_t j = offsets[i]; j < offsets[i + 1]; j++) {
      edges[j] = G[i][j - offsets[i]].first;
      weights[j] = G[i][j - offsets[i]].second;
    }
  }
  assert(offsets[n] == m);

  parlay::chars graph_chars;
  graph_chars.insert(graph_chars.end(),
                     parlay::to_chars("WeightedAdjacencyGraph\n"));
  graph_chars.insert(graph_chars.end(),
                     parlay::to_chars(std::to_string(n) + "\n"));
  graph_chars.insert(graph_chars.end(),
                     parlay::to_chars(std::to_string(m) + "\n"));
  graph_chars.insert(graph_chars.end(),
                     parlay::flatten(parlay::tabulate(n * 2, [&](size_t i) {
                       if (i % 2 == 0) {
                         return parlay::to_chars(offsets[i / 2]);
                       } else {
                         return parlay::to_chars('\n');
                       }
                     })));
  graph_chars.insert(graph_chars.end(),
                     parlay::flatten(parlay::tabulate(m * 2, [&](size_t i) {
                       if (i % 2 == 0) {
                         return parlay::to_chars(edges[i / 2]);
                       } else {
                         return parlay::to_chars('\n');
                       }
                     })));
  graph_chars.insert(
      graph_chars.end(), parlay::flatten(parlay::tabulate(m * 2, [&](size_t i) {
        if (i % 2 == 0) {
          return parlay::to_chars(std::to_string(weights[i / 2]));
        } else {
          return parlay::to_chars('\n');
        }
      })));
  chars_to_file(graph_chars, "graph.adj");

  printf("n: %zu, m: %zu\n", n, m);
  printf("mx: %f, mn: %f, avg: %f\n", mx, mn, avg / m);
  return 0;
}
