import pandas as pd
import sys
import matplotlib.pyplot as plt


if len(sys.argv) <= 2:
    print("please give at least 1 input file along with the output file")

dfs = []
for fname in sys.argv[2:]:
    dfs.append((fname, pd.read_csv(fname, skipinitialspace=True)))

output_prefix = sys.argv[1]

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
