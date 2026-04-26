import pandas as pd
import sys
import matplotlib.pyplot as plt


if len(sys.argv) != 4:
    print("give the input file, output file, and title")


df = pd.read_csv(sys.argv[1], skipinitialspace=True)

output_filename = sys.argv[2]


df.plot(x="timestep", legend=False)
plt.ylim(ymin=0)
plt.title(sys.argv[3])
plt.savefig(output_filename, bbox_inches="tight")
