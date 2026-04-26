import struct
import argparse
from collections import defaultdict

"""
This script converts plain-text edge list graphs into the binary format.

SUPPORTED GRAPH FORMAT (INPUT):
- Text file with one edge per line in the format:
    <src_id> <dest_id> <timestamp>
- All values must be unsigned 32-bit integers (uint32_t).
- No weights are supported (weight_type = None).
- The graph is treated as a signal graph (if_signal_graph = true).
- The input graph vertex ids do not need to be packed into the range (0, N-1).
"""

UINT32 = 12
NONE = 0

def build_vertex_map(filename):
    vertices = set()
    with open(filename, 'r') as f:
        for line in f:
            if not line.strip():
                continue
            src, dst, _ = map(int, line.strip().split())
            vertices.add(src)
            vertices.add(dst)
    id_map = {v: i for i, v in enumerate(sorted(vertices))}
    return id_map

def count_unique_edges(filename):
    edge_set = set()
    with open(filename, 'r') as f:
        for line in f:
            if not line.strip():
                continue
            src, dst, _ = map(int, line.strip().split())
            edge_set.add((src, dst))
    return len(edge_set)

def write_binary(input_txt, output_bin, id_map, num_unique_edges):
    num_vertices = len(id_map)
    total_updates = 0

    with open(output_bin, 'wb') as out:
        # Write placeholder header
        out.write(struct.pack('<Q', num_vertices))
        out.write(struct.pack('<Q', num_unique_edges))
        out.write(struct.pack('<Q', 0))  # will update total_updates later
        out.write(struct.pack('<B', 1))  # if_signal_graph = true
        out.write(struct.pack('<B', UINT32))  # vertex_id_type
        out.write(struct.pack('<B', UINT32))  # timestamp_type
        out.write(struct.pack('<B', 1))  # if_directed = true
        out.write(struct.pack('<B', NONE))  # weight_type
        out.write(b'\x00' * (32 - 8*3 - 5))  # padding to 32 bytes

        for line in open(input_txt, 'r'):
            if not line.strip():
                continue
            src, dst, ts = map(int, line.strip().split())
            out.write(struct.pack('<III', id_map[src], id_map[dst], ts))
            total_updates += 1

        # Seek back to write total_updates
        out.seek(8 * 2)  # Skip num_vertices, num_unique_edges
        out.write(struct.pack('<Q', total_updates))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('input_txt', help='Input edge list (text format)')
    parser.add_argument('output_bin', help='Output binary file')
    args = parser.parse_args()

    print("Building vertex map...")
    id_map = build_vertex_map(args.input_txt)

    print("Counting unique edges...")
    num_unique_edges = count_unique_edges(args.input_txt)

    print("Writing binary file...")
    write_binary(args.input_txt, args.output_bin, id_map, num_unique_edges)

    print(f"Done. {len(id_map)} vertices, {num_unique_edges} unique edges.")
