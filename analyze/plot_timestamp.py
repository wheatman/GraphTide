import pandas as pd
import sys
import matplotlib.pyplot as plt


if len(sys.argv) != 3:
    print("give the input file, output file")


df = pd.read_csv(sys.argv[1], skipinitialspace=True)

output_filename = sys.argv[2]


for field in ["num_nodes", "num_edges", "average_degree", "max_degree", "new_edges", "num_triangles", "num_components", "largest_component"]:
# for field in ["component_count", "max_component_size"]:
    df.plot(x="timestamp", y=field, legend=False)
    plt.ylim(ymin=0)
    plt.title(output_filename.split("/")[-1]+" "+field+" timestamp")
    plt.savefig(output_filename+"_"+field +
                "_timestamp.png", bbox_inches="tight")
