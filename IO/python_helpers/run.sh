#!/bin/bash

INPUT_DIR="../../graphs/dynamic_graphs_parsed"
OUTPUT_DIR="../../graphs/bin_dynamic_graphs"

FILES=(
  "soc-bitcoin.edges"
  "rec-amazon-ratings.edges"
  "rec-amz-Home-and-Kitchen.edges"
  "soc-flickr-growth.edges"
  "rec-amz-Books.edges"
  "rec-amz-Movies-and-TV.edges"
  "soc-youtube-growth.edges"
  "rec-amz-CDs-and-Vinyl.edges"
  "rec-amz-Sports-and-Outdoors.edges"
  "sx-stackoverflow.edges"
)

for filename in "${FILES[@]}"; do
  input_path="$INPUT_DIR/$filename"
  output_name="${filename/.edges/.bin}"
  output_path="$OUTPUT_DIR/$output_name"

  echo "Converting $filename..."
  python convert_basic.py "$input_path" "$output_path"
done