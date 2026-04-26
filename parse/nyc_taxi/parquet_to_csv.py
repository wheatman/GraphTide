import sys
import os
import pandas as pd
import pyarrow.parquet as pq
input_file = sys.argv[1]
trips = pq.read_table(input_file)
trips = trips.to_pandas()

output_file = os.path.splitext(input_file)[0] + '.csv'
trips.to_csv(output_file, index=False)
