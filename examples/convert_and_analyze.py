import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))

import pandas as pd
from pybinding import convert, read_bin, analyze

csv_path = sys.argv[1]

# Step 1: Read CSV and convert to binary
df = pd.read_csv(
    csv_path,
    skiprows=1,
    header=None,
    names=["src", "dst", "timestamp", "info"],
)
print("DataFrame shape:", df.shape)
print(df.head())

bin_path = "/tmp/pybinding_test.bin"
convert(df, bin_path,
    signal=False,
    directed=True,
    timestamp_type="uint32",
)

# Step 2: Read back and verify
h, data = read_bin(bin_path)
print("\nHeader:", h)
print("First 5 rows:", data[:5])

# Step 3: Run benchmarks
result = analyze(
    bin_path,
    output_dir="/tmp/pybinding_analysis",
    print_freq=1000,
    static_algorithm_count=5,
    src=1,
)

print("\nDynamic stats:")
print(result.dynamic)

print("\nStatic stats:")
print(result.static)


