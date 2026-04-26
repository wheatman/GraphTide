# pybinding

Python bindings for the Dynamic Graph Dataset Collection binary format.

## Usage

**Option 1**: Run example scripts directly from the repo (no setup needed):

```bash
python3 examples/convert_and_analyze.py examples/antarctica.csv
```

Example scripts automatically add the repo root to `sys.path`.

**Option 2**: Set `PYTHONPATH` for your own scripts:

```bash
PYTHONPATH=/path/to/repo:$PYTHONPATH python3 your_script.py
```

**Option 3**: Install as a package:

```bash
pip install -e .
```

## Modules

### `pybinding.io` -- Read binary graph files

```python
from pybinding import read_header, read_bin

# Inspect header only (fast, no data loaded)
header = read_header("graph.bin")
# {'num_vertices': 8848, 'num_unique_edges': 7216, 'total_updates': 13831,
#  'signal': False, 'vertex_id_type': 'UINT32', 'timestamp_type': 'UINT32',
#  'directed': True, 'weight_type': 'FLOAT64'}

# Read header + all edge updates as a numpy structured array
header, data = read_bin("graph.bin")
# data.dtype.names = ('src', 'dst', 'timestamp', 'weight', 'info')
# data['info']: 0 = INSERT, 1 = DELETE
```

### `pybinding.convert` -- Write binary graph files

Convert a pandas DataFrame or CSV file to the binary format.

```python
from pybinding import convert
import pandas as pd

# From a DataFrame
df = pd.DataFrame({
    "src": [0, 1, 2, 0],
    "dst": [1, 2, 3, 3],
    "timestamp": [100, 200, 300, 400],
    "weight": [1.5, 2.0, 3.0, 0.0],
    "info": ["insert", "insert", "insert", "delete"],
})
convert(df, "output.bin",
        directed=True,
        signal=False,
        vertex_id_type="uint32",
        timestamp_type="uint32",
        weight_type="float64")

# Signal graph (insert-only, no info column needed)
df_signal = pd.DataFrame({
    "src": ["alice", "bob", "alice"],
    "dst": ["bob", "carol", "carol"],
    "timestamp": [1, 2, 3],
})
convert(df_signal, "signal.bin", signal=True, timestamp_type="uint32")
# Vertex IDs are automatically remapped to contiguous integers

# From a CSV file
convert("edges.csv", "output.bin", timestamp_type="uint32")
```

### `pybinding.analyze` -- Run graph algorithms

Thin wrapper around the C++ `graph_statistics` binary. Returns results as DataFrames.

```python
from pybinding import analyze

result = analyze("graph.bin",
    print_freq=1000,
    static_algorithm_count=10,
    src=1)

result.dynamic   # DataFrame: timestep, timestamp, num_nodes, num_edges,
                 #            average_degree, max_degree, new_edges,
                 #            component_count, max_component_size, num_triangles

result.static    # DataFrame: timestep, timestamp, max_distance_from_src,
                 #            component_count, max_component_size,
                 #            avg_coreness, max_coreness, ...
```

**Prerequisite:** Build the analysis binary first:

```bash
cd analyze
mkdir -p build && cd build
cmake .. && make -j
```

## Binary Format

See [IO/README.md](../IO/README.md) for the full binary format specification.

Summary: 32-byte header followed by tightly-packed rows.

| Field | Type | Present when |
|---|---|---|
| `src` | `vertex_id_type` | Always |
| `dst` | `vertex_id_type` | Always |
| `timestamp` | `timestamp_type` | `timestamp_type != NONE` |
| `weight` | `weight_type` | `weight_type != NONE` |
| `info` | `uint8` (0=INSERT, 1=DELETE) | `signal == False` |

## Supported DataTypes

`NONE`, `UINT8`, `UINT16`, `UINT32`, `UINT64`, `INT8`, `INT16`, `INT32`, `INT64`, `FLOAT32`, `FLOAT64`
