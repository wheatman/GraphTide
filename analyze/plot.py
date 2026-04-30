import pandas as pd
import sys
import matplotlib.pyplot as plt


USAGE = "Usage: python3 plot.py <input_csv_1> [<input_csv_2> ...] <output_prefix>"

dfs = []
if len(sys.argv) < 3:
    print(USAGE)
    sys.exit(1)

input_files = sys.argv[1:-1]
output_prefix = sys.argv[-1]

for fname in input_files:
    dfs.append((fname, pd.read_csv(fname, skipinitialspace=True)))

for field in ["num_nodes", "num_edges", "average_degree", "max_degree", "num_triangles", "new_edges"]:
    longest_df = dfs[0][1]
    for d in dfs[1:]:
        if len(d[1].index) > len(longest_df.index):
            longest_df = d[1]
    df = pd.DataFrame(longest_df["timestep"])
    for d in dfs:
        df[d[0]] = d[1][field]
    df.plot(x="timestep", legend=False)
    plt.ylim(ymin=0)
    plt.title(output_prefix.split("/")[-1]+"_"+field)
    plt.savefig(output_prefix+"_"+field+".png", bbox_inches="tight")
