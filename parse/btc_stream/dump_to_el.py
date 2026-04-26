

import pandas as pd
import os

def process_files(output_file1, output_file2, files):
    """
    Reads all CSV files in a directory into Pandas DataFrames, extracts selected columns,
    and appends the subset to a single CSV file.

    :param input_dir: Directory containing input CSV files
    :param output_file: Path to save the combined output CSV file
    :param selected_columns: List of columns to retain in the output
    """
    all_data = []
    
    for file in files:
        try:
            df = pd.read_parquet(file)
            all_data.append(df)
            print(f"Processed {file}")
        except Exception as e:
            print(f"Error processing {file}: {e}")
    if all_data:
        combined_df = pd.concat(all_data, ignore_index=True)
        combined_df[["SRC_ID", "DST_ID", "TIMESTAMP"]].to_csv(output_file1, index=False, header=False, sep='\t')
        combined_df[["SRC_ID", "DST_ID", "TIMESTAMP", "VALUE_SATOSHI"]].to_csv(output_file2, index=False, header=False, sep='\t')

# Example usage
input_files = [
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2009-file-id-01.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2010-file-id-02.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2011-file-id-03.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2012-file-id-04.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2013-file-id-05.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2014-file-id-06.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2015-file-id-07.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2016-file-id-08.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2017-file-id-09.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2018-file-id-10.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2019-file-id-11.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2020-file-id-12.snappy.parquet",
"STREAM_GRAPH/EDGES/orbitaal-stream_graph-date-2021-file-id-13.snappy.parquet"
]
process_files("btc_stream.edges", "btc_stream_weighted.edges", input_files)
