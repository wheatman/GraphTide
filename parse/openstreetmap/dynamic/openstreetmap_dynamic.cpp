#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <limits>
#include <stdexcept>
#include <utility>

using NodeID = osmium::object_id_type;
using WayID = osmium::object_id_type;
using SegmentKey = std::pair<NodeID, NodeID>;

struct SegmentKeyHash {
    size_t operator()(const SegmentKey& p) const {
        size_t h1 = std::hash<NodeID>{}(p.first);
        size_t h2 = std::hash<NodeID>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

// ---------------------------------------------------------------------------
// Accessibility enums and edge properties (matching osm4routing2/categorize.rs)
// ---------------------------------------------------------------------------

enum class FootAccess { Unknown, Forbidden, Allowed };
enum class CarAccess { Unknown, Forbidden, Residential, Tertiary, Secondary, Primary, Trunk, Motorway };
enum class BikeAccess { Unknown, Forbidden, Allowed, Lane, Busway, Track };
enum class TrainAccess { Unknown, Forbidden, Allowed };

struct EdgeProperties {
    FootAccess foot = FootAccess::Unknown;
    CarAccess car_forward = CarAccess::Unknown;
    CarAccess car_backward = CarAccess::Unknown;
    BikeAccess bike_forward = BikeAccess::Unknown;
    BikeAccess bike_backward = BikeAccess::Unknown;
    TrainAccess train = TrainAccess::Unknown;

    void apply_tag(const std::string& key, const std::string& val) {
        if (key == "highway") {
            if (val == "cycleway") {
                bike_forward = BikeAccess::Track;
                foot = FootAccess::Allowed;
            } else if (val == "path" || val == "footway" || val == "steps" || val == "pedestrian") {
                bike_forward = BikeAccess::Allowed;
                foot = FootAccess::Allowed;
            } else if (val == "primary" || val == "primary_link") {
                car_forward = CarAccess::Primary;
                foot = FootAccess::Allowed;
                bike_forward = BikeAccess::Allowed;
            } else if (val == "secondary" || val == "secondary_link") {
                car_forward = CarAccess::Secondary;
                foot = FootAccess::Allowed;
                bike_forward = BikeAccess::Allowed;
            } else if (val == "tertiary" || val == "tertiary_link") {
                car_forward = CarAccess::Tertiary;
                foot = FootAccess::Allowed;
                bike_forward = BikeAccess::Allowed;
            } else if (val == "unclassified" || val == "residential" || val == "living_street" ||
                       val == "road" || val == "service" || val == "track") {
                car_forward = CarAccess::Residential;
                foot = FootAccess::Allowed;
                bike_forward = BikeAccess::Allowed;
            } else if (val == "motorway" || val == "motorway_link" || val == "motorway_junction") {
                car_forward = CarAccess::Motorway;
                foot = FootAccess::Forbidden;
                bike_forward = BikeAccess::Forbidden;
            } else if (val == "trunk" || val == "trunk_link") {
                car_forward = CarAccess::Trunk;
                foot = FootAccess::Forbidden;
                bike_forward = BikeAccess::Forbidden;
            }
        } else if (key == "pedestrian" || key == "foot") {
            foot = (val == "no") ? FootAccess::Forbidden : FootAccess::Allowed;
        } else if (key == "cycleway") {
            if (val == "track") bike_forward = BikeAccess::Track;
            else if (val == "opposite_track") bike_backward = BikeAccess::Track;
            else if (val == "opposite") bike_backward = BikeAccess::Allowed;
            else if (val == "share_busway") bike_forward = BikeAccess::Busway;
            else if (val == "lane_left" || val == "opposite_lane") bike_backward = BikeAccess::Lane;
            else bike_forward = BikeAccess::Lane;
        } else if (key == "bicycle") {
            bike_forward = (val == "no" || val == "false") ? BikeAccess::Forbidden : BikeAccess::Allowed;
        } else if (key == "busway") {
            if (val == "opposite_lane" || val == "opposite_track") bike_backward = BikeAccess::Busway;
            else bike_forward = BikeAccess::Busway;
        } else if (key == "oneway" && (val == "yes" || val == "true" || val == "1")) {
            car_backward = CarAccess::Forbidden;
            if (bike_backward == BikeAccess::Unknown) bike_backward = BikeAccess::Forbidden;
        } else if (key == "junction" && val == "roundabout") {
            car_backward = CarAccess::Forbidden;
            if (bike_backward == BikeAccess::Unknown) bike_backward = BikeAccess::Forbidden;
        } else if (key == "railway") {
            train = TrainAccess::Allowed;
        }
    }

    void normalize() {
        if (car_backward == CarAccess::Unknown) car_backward = car_forward;
        if (bike_backward == BikeAccess::Unknown) bike_backward = bike_forward;
        if (car_forward == CarAccess::Unknown) car_forward = CarAccess::Forbidden;
        if (bike_forward == BikeAccess::Unknown) bike_forward = BikeAccess::Forbidden;
        if (car_backward == CarAccess::Unknown) car_backward = CarAccess::Forbidden;
        if (bike_backward == BikeAccess::Unknown) bike_backward = BikeAccess::Forbidden;
        if (foot == FootAccess::Unknown) foot = FootAccess::Forbidden;
        if (train == TrainAccess::Unknown) train = TrainAccess::Forbidden;
    }

    bool accessible() const {
        return bike_forward != BikeAccess::Forbidden ||
               bike_backward != BikeAccess::Forbidden ||
               car_forward != CarAccess::Forbidden ||
               car_backward != CarAccess::Forbidden ||
               foot != FootAccess::Forbidden ||
               train != TrainAccess::Forbidden;
    }

    bool operator==(const EdgeProperties& o) const {
        return foot == o.foot && car_forward == o.car_forward &&
               car_backward == o.car_backward && bike_forward == o.bike_forward &&
               bike_backward == o.bike_backward && train == o.train;
    }
};

// ---------------------------------------------------------------------------
// Node indexer: maps OSM node IDs to contiguous uint32_t indices
// ---------------------------------------------------------------------------

class NodeIndexer {
public:
    uint32_t get_index(NodeID id) {
        auto it = indices.find(id);
        if (it != indices.end()) return it->second;
        if (indices.size() >= static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
            throw std::runtime_error("Too many nodes to index into uint32_t");
        }
        uint32_t next = static_cast<uint32_t>(indices.size());
        indices.emplace(id, next);
        return next;
    }
    size_t size() const { return indices.size(); }
private:
    std::unordered_map<NodeID, uint32_t> indices;
};

// ---------------------------------------------------------------------------
// Edge update record
// ---------------------------------------------------------------------------

struct EdgeUpdate {
    uint32_t u;
    uint32_t v;
    int64_t timestamp;
    bool insertion;
    double weight;
};

// ---------------------------------------------------------------------------
// Segment: edge between two intersection nodes, with full intermediate path
// ---------------------------------------------------------------------------

struct Segment {
    SegmentKey key;
    std::vector<NodeID> path;
};

// ---------------------------------------------------------------------------
// Haversine distance (earth radius matches osm4routing2)
// ---------------------------------------------------------------------------

double haversine_distance(double lat_a, double lon_a, double lat_b, double lon_b) {
    constexpr double pi = 3.14159265358979323846;
    constexpr double deg_to_rad = pi / 180.0;
    constexpr double earth_radius = 6378100.0;
    double dlat = (lat_b - lat_a) * deg_to_rad;
    double dlon = (lon_b - lon_a) * deg_to_rad;
    double sin_dlat = std::sin(dlat / 2.0);
    double sin_dlon = std::sin(dlon / 2.0);
    double a = sin_dlat * sin_dlat +
               std::cos(lat_a * deg_to_rad) * std::cos(lat_b * deg_to_rad) *
               sin_dlon * sin_dlon;
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return earth_radius * c;
}

// ---------------------------------------------------------------------------
// Timestamp conversion
// ---------------------------------------------------------------------------

int64_t to_unix_timestamp(const osmium::Timestamp& ts) {
    if (!ts) throw std::runtime_error("Undefined timestamp encountered");
    const std::time_t seconds = ts.seconds_since_epoch();
    if (seconds < 0) throw std::runtime_error("Timestamp is before Unix epoch");
    return static_cast<int64_t>(seconds);
}

// =========================================================================
// Pass 1: Scan full history to determine final way state and node locations
// =========================================================================

struct FinalWay {
    std::vector<NodeID> nodes;
    EdgeProperties props;
};

class FinalStateScanner : public osmium::handler::Handler {
public:
    std::unordered_map<NodeID, osmium::Location> node_locations;
    std::unordered_map<WayID, FinalWay> final_ways;

    void node(const osmium::Node& node) {
        if (node.location().valid()) {
            node_locations[node.id()] = node.location();
        }
    }

    void way(const osmium::Way& way) {
        EdgeProperties props;
        for (const auto& tag : way.tags()) {
            props.apply_tag(tag.key(), tag.value());
        }
        props.normalize();

        WayID id = way.id();
        if (!way.visible() || !props.accessible()) {
            final_ways.erase(id);
            return;
        }

        std::vector<NodeID> nodes;
        nodes.reserve(way.nodes().size());
        for (const auto& nr : way.nodes()) {
            nodes.push_back(nr.ref());
        }
        final_ways[id] = FinalWay{std::move(nodes), props};
    }
};

// ---------------------------------------------------------------------------
// Compute intersection nodes from final active ways (osm4routing2 algorithm:
// extremity nodes count +2, interior nodes count +1, keep nodes with uses > 1)
// ---------------------------------------------------------------------------

std::unordered_set<NodeID> compute_intersections(
    const std::unordered_map<WayID, FinalWay>& final_ways
) {
    std::unordered_map<NodeID, int> uses;
    for (const auto& [way_id, fw] : final_ways) {
        for (size_t i = 0; i < fw.nodes.size(); ++i) {
            if (i == 0 || i == fw.nodes.size() - 1) {
                uses[fw.nodes[i]] += 2;
            } else {
                uses[fw.nodes[i]] += 1;
            }
        }
    }
    std::unordered_set<NodeID> result;
    for (const auto& [nid, count] : uses) {
        if (count > 1) {
            result.insert(nid);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Merge edges: remove degree-2 intersection nodes where the two edges meeting
// at the node have identical EdgeProperties (matching osm4routing2 --merge-edges).
// Faithful reimplementation of osm4routing2's do_merge_edges: iterative rounds
// that merge pairs at uses==4 nodes until no more merges are possible.
// ---------------------------------------------------------------------------

struct SplitEdge {
    NodeID source;
    NodeID target;
    EdgeProperties props;
    size_t id;  // unique edge id for dedup
};

void merge_intersections(
    std::unordered_set<NodeID>& intersections,
    const std::unordered_map<WayID, FinalWay>& final_ways
) {
    // Build edges by splitting final ways at intersections
    std::vector<SplitEdge> edges;
    size_t next_id = 0;
    for (const auto& [wid, fw] : final_ways) {
        if (fw.nodes.size() < 2) continue;
        NodeID source = fw.nodes[0];
        for (size_t i = 1; i < fw.nodes.size(); ++i) {
            NodeID curr = fw.nodes[i];
            if (curr == source) continue;
            if (intersections.count(curr) || i == fw.nodes.size() - 1) {
                edges.push_back(SplitEdge{source, curr, fw.props, next_id++});
                source = curr;
            }
        }
    }

    std::cout << "    merge: " << edges.size() << " initial split edges" << std::endl;

    // Compute uses: each edge endpoint counts +2 (matching osm4routing2 extremity counting)
    std::unordered_map<NodeID, int> uses;
    for (const auto& e : edges) {
        uses[e.source] += 2;
        uses[e.target] += 2;
    }

    size_t total_removed = 0;
    int round = 0;
    bool changed = true;
    while (changed) {
        changed = false;
        round++;

        // Build adjacency for candidate nodes (uses == 4)
        std::unordered_map<NodeID, std::vector<size_t>> neighbors;
        neighbors.reserve(edges.size());
        for (size_t i = 0; i < edges.size(); ++i) {
            auto it_s = uses.find(edges[i].source);
            if (it_s != uses.end() && it_s->second == 4) {
                neighbors[edges[i].source].push_back(i);
            }
            auto it_t = uses.find(edges[i].target);
            if (it_t != uses.end() && it_t->second == 4) {
                neighbors[edges[i].target].push_back(i);
            }
        }

        std::unordered_set<size_t> merged_ids;
        std::vector<SplitEdge> new_edges;
        size_t round_merges = 0;

        for (auto& [node, idxs] : neighbors) {
            if (idxs.size() != 2) continue;
            size_t a = idxs[0], b = idxs[1];
            if (merged_ids.count(edges[a].id) || merged_ids.count(edges[b].id)) continue;
            if (!(edges[a].props == edges[b].props)) continue;
            if (edges[a].id == edges[b].id) continue;

            NodeID src_a = (edges[a].source == node) ? edges[a].target : edges[a].source;
            NodeID src_b = (edges[b].source == node) ? edges[b].target : edges[b].source;
            new_edges.push_back(SplitEdge{src_a, src_b, edges[a].props, next_id++});

            merged_ids.insert(edges[a].id);
            merged_ids.insert(edges[b].id);
            intersections.erase(node);
            round_merges++;
            changed = true;
        }

        for (size_t i = 0; i < edges.size(); ++i) {
            if (!merged_ids.count(edges[i].id)) {
                new_edges.push_back(std::move(edges[i]));
            }
        }
        edges = std::move(new_edges);

        if (changed) {
            uses.clear();
            for (const auto& e : edges) {
                uses[e.source] += 2;
                uses[e.target] += 2;
            }
        }
        total_removed += round_merges;
        std::cout << "    merge round " << round << ": removed " << round_merges
                  << " nodes (" << edges.size() << " edges remaining)" << std::endl;
    }
    std::cout << "    merge complete: removed " << total_removed << " degree-2 nodes in "
              << round << " rounds" << std::endl;
}

// ---------------------------------------------------------------------------
// Split a way's node list at intersection nodes (or at way endpoints)
// ---------------------------------------------------------------------------

std::vector<Segment> split_way_at_intersections(
    const osmium::WayNodeList& way_nodes,
    const std::unordered_set<NodeID>& intersections
) {
    std::vector<NodeID> nodes;
    nodes.reserve(way_nodes.size());
    for (const auto& nr : way_nodes) {
        NodeID nid = nr.ref();
        if (nodes.empty() || nid != nodes.back()) {
            nodes.push_back(nid);
        }
    }

    std::vector<Segment> segments;
    if (nodes.size() < 2) return segments;

    NodeID source = nodes[0];
    std::vector<NodeID> path = {source};

    for (size_t i = 1; i < nodes.size(); ++i) {
        NodeID curr = nodes[i];
        path.push_back(curr);

        if (intersections.count(curr) || i == nodes.size() - 1) {
            if (path.size() >= 2) {
                segments.push_back(Segment{SegmentKey{source, curr}, std::move(path)});
            }
            source = curr;
            path = {curr};
        }
    }
    return segments;
}

// =========================================================================
// Pass 2: Process history with intersection-aware splitting
// =========================================================================

class HistoryExtractor : public osmium::handler::Handler {
public:
    HistoryExtractor(
        NodeIndexer& indexer_ref,
        const std::unordered_set<NodeID>& intersection_set,
        const std::unordered_map<NodeID, osmium::Location>& locations,
        bool weighted
    ) : indexer(indexer_ref),
        intersections(intersection_set),
        node_locations(locations),
        is_weighted(weighted) {}

    void way(const osmium::Way& way) {
        EdgeProperties props;
        for (const auto& tag : way.tags()) {
            props.apply_tag(tag.key(), tag.value());
        }
        props.normalize();

        WayID id = way.id();
        int64_t timestamp = to_unix_timestamp(way.timestamp());

        if (!way.visible() || !props.accessible()) {
            auto it = current_segments.find(id);
            if (it != current_segments.end()) {
                remove_segments(it->second, timestamp);
                current_segments.erase(it);
            }
            return;
        }

        std::vector<Segment> segs = split_way_at_intersections(way.nodes(), intersections);
        if (segs.empty()) return;

        auto it = current_segments.find(id);
        if (it == current_segments.end()) {
            add_segments(segs, timestamp);
            current_segments.emplace(id, std::move(segs));
        } else {
            apply_diff(it->second, segs, timestamp);
            it->second = std::move(segs);
        }
    }

    size_t total_vertices() const { return indexer.size(); }
    size_t total_unique_edges() const { return unique_edges.size(); }
    const std::vector<EdgeUpdate>& updates_list() const { return updates; }
    size_t insertion_count() const { return insertion_events; }
    size_t deletion_count() const { return deletion_events; }
    size_t modification_count() const { return modification_events; }
    size_t skipped_count() const { return skipped_missing_location; }

private:
    NodeIndexer& indexer;
    const std::unordered_set<NodeID>& intersections;
    const std::unordered_map<NodeID, osmium::Location>& node_locations;
    bool is_weighted;

    std::unordered_map<WayID, std::vector<Segment>> current_segments;
    std::unordered_map<SegmentKey, int, SegmentKeyHash> segment_use;
    std::unordered_set<SegmentKey, SegmentKeyHash> unique_edges;
    std::vector<EdgeUpdate> updates;
    size_t insertion_events = 0;
    size_t deletion_events = 0;
    size_t modification_events = 0;
    size_t skipped_missing_location = 0;

    void add_segments(const std::vector<Segment>& segs, int64_t timestamp) {
        for (const auto& seg : segs) insert_segment(seg, timestamp);
    }

    void remove_segments(const std::vector<Segment>& segs, int64_t timestamp) {
        for (const auto& seg : segs) erase_segment(seg.key, timestamp);
    }

    void apply_diff(const std::vector<Segment>& before,
                    const std::vector<Segment>& after,
                    int64_t timestamp) {
        std::unordered_map<SegmentKey, int, SegmentKeyHash> counts_before;
        std::unordered_map<SegmentKey, int, SegmentKeyHash> counts_after;
        std::unordered_map<SegmentKey, const std::vector<NodeID>*, SegmentKeyHash> after_paths;

        for (const auto& seg : before) counts_before[seg.key]++;
        for (const auto& seg : after) {
            counts_after[seg.key]++;
            after_paths[seg.key] = &seg.path;
        }

        bool changed = false;
        for (const auto& [key, ac] : counts_after) {
            int bc = counts_before[key];
            if (ac > bc) {
                for (int i = 0; i < ac - bc; ++i) {
                    insert_segment(Segment{key, *after_paths.at(key)}, timestamp);
                    changed = true;
                }
            }
        }
        for (const auto& [key, bc] : counts_before) {
            int ac = counts_after[key];
            if (bc > ac) {
                for (int i = 0; i < bc - ac; ++i) {
                    erase_segment(key, timestamp);
                    changed = true;
                }
            }
        }
        if (changed) modification_events++;
    }

    void insert_segment(const Segment& seg, int64_t timestamp) {
        double w = 1.0;
        if (is_weighted) {
            w = compute_path_weight(seg.path);
            if (std::isnan(w)) {
                skipped_missing_location++;
                return;
            }
        }
        segment_use[seg.key]++;
        unique_edges.insert(seg.key);
        uint32_t u = indexer.get_index(seg.key.first);
        uint32_t v = indexer.get_index(seg.key.second);
        updates.push_back(EdgeUpdate{u, v, timestamp, true, w});
        insertion_events++;
    }

    void erase_segment(const SegmentKey& key, int64_t timestamp) {
        auto it = segment_use.find(key);
        if (it == segment_use.end()) return;
        it->second--;
        if (it->second <= 0) segment_use.erase(it);
        uint32_t u = indexer.get_index(key.first);
        uint32_t v = indexer.get_index(key.second);
        updates.push_back(EdgeUpdate{u, v, timestamp, false, 0.0});
        deletion_events++;
    }

    double compute_path_weight(const std::vector<NodeID>& path) {
        double total = 0.0;
        for (size_t i = 1; i < path.size(); ++i) {
            auto a = node_locations.find(path[i - 1]);
            auto b = node_locations.find(path[i]);
            if (a == node_locations.end() || b == node_locations.end() ||
                !a->second.valid() || !b->second.valid()) {
                NodeID bad = (a == node_locations.end() || (a != node_locations.end() && !a->second.valid()))
                    ? path[i - 1] : path[i];
                if (skipped_missing_location < 5) {
                    std::cerr << "Warning: missing/invalid location for node "
                              << static_cast<long long>(bad)
                              << ", skipping edge" << std::endl;
                }
                return std::numeric_limits<double>::quiet_NaN();
            }
            total += haversine_distance(
                a->second.lat(), a->second.lon(),
                b->second.lat(), b->second.lon());
        }
        return total;
    }
};

// ---------------------------------------------------------------------------
// CLI
// ---------------------------------------------------------------------------

struct Options {
    std::string input;
    std::string output;
    bool weighted = false;
    bool merge_edges = false;
};

Options parse_cli(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0]
                  << " history.osm.pbf --output updates.csv [--weighted] [--merge-edges]"
                  << std::endl;
        std::exit(1);
    }
    Options opts;
    opts.input = argv[1];
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--output" && i + 1 < argc) {
            opts.output = argv[++i];
        } else if (arg == "--weighted") {
            opts.weighted = true;
        } else if (arg == "--merge-edges") {
            opts.merge_edges = true;
        }
    }
    if (opts.output.empty()) {
        std::cerr << "--output is required." << std::endl;
        std::exit(1);
    }
    return opts;
}

// ---------------------------------------------------------------------------
// Main: two-pass processing
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    Options opts = parse_cli(argc, argv);

    // Pass 1: scan full history for final way state and all node locations
    std::cout << "Pass 1: scanning for final state and node locations..." << std::endl;
    FinalStateScanner scanner;
    {
        osmium::io::Reader reader{opts.input};
        osmium::apply(reader, scanner);
        reader.close();
    }
    std::cout << "  Node locations: " << scanner.node_locations.size()
              << ", Final active ways: " << scanner.final_ways.size() << std::endl;

    // Compute intersection nodes (osm4routing2 algorithm)
    auto intersections = compute_intersections(scanner.final_ways);
    std::cout << "  Intersection nodes: " << intersections.size() << std::endl;

    if (opts.merge_edges) {
        size_t before = intersections.size();
        merge_intersections(intersections, scanner.final_ways);
        std::cout << "  After --merge-edges: " << intersections.size()
                  << " (removed " << (before - intersections.size()) << " degree-2 nodes)" << std::endl;
    }
    scanner.final_ways.clear();

    // Pass 2: replay history with intersection-aware splitting
    std::cout << "Pass 2: extracting temporal edge updates"
              << (opts.weighted ? " (weighted)" : " (unweighted)") << "..." << std::endl;
    NodeIndexer indexer;
    HistoryExtractor extractor(indexer, intersections, scanner.node_locations, opts.weighted);
    {
        osmium::io::Reader reader{opts.input};
        osmium::apply(reader, extractor);
        reader.close();
    }

    // Write output
    std::ofstream out(opts.output);
    if (!out.is_open()) {
        std::cerr << "Failed to open output file: " << opts.output << std::endl;
        return 1;
    }

    const auto& updates = extractor.updates_list();
    out << extractor.total_vertices() << "," << extractor.total_unique_edges()
        << "," << updates.size() << "\n";
    for (const auto& upd : updates) {
        out << upd.u << "," << upd.v << "," << upd.timestamp << ","
            << (upd.insertion ? "insert" : "delete");
        if (opts.weighted && upd.insertion) {
            out << "," << upd.weight;
        }
        out << "\n";
    }

    std::cout << "Wrote " << updates.size() << " updates to " << opts.output << std::endl;
    std::cout << "Vertices: " << extractor.total_vertices()
              << ", Unique edges: " << extractor.total_unique_edges()
              << ", Updates: " << updates.size() << std::endl;
    std::cout << "Insertions: " << extractor.insertion_count()
              << ", Deletions: " << extractor.deletion_count()
              << ", Modifications: " << extractor.modification_count() << std::endl;
    if (extractor.skipped_count() > 0) {
        std::cout << "Skipped (missing node location): " << extractor.skipped_count() << std::endl;
    }
    return 0;
}
