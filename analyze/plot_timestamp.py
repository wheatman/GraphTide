import pandas as pd
import sys
import matplotlib.pyplot as plt

USAGE = "Usage: python3 plot_timestamp.py <input_csv> <output_prefix>"

if len(sys.argv) != 3:
    print(USAGE)
    sys.exit(1)


df = pd.read_csv(sys.argv[1], skipinitialspace=True)

output_filename = sys.argv[2]

# Support both old/new naming conventions for component stats.
component_count_field = "num_components" if "num_components" in df.columns else "component_count"
largest_component_field = "largest_component" if "largest_component" in df.columns else "max_component_size"

for field in ["num_nodes", "num_edges", "average_degree", "max_degree", "new_edges", "num_triangles", component_count_field, largest_component_field]:
    if field not in df.columns:
        continue
    df.plot(x="timestamp", y=field, legend=False)
    plt.ylim(ymin=0)
    plt.title(output_filename.split("/")[-1]+" "+field+" timestamp")
    plt.savefig(output_filename+"_"+field +
                "_timestamp.png", bbox_inches="tight")
