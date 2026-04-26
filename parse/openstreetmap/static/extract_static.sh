#!/bin/bash

declare -a continents=(
  "antarctica-latest"
)

for continent in ${continents[@]}; do
  osm4routing pbf/$continent.osm.pbf
  #cp nodes.csv raw/${continent}_nodes.csv
  #cp edges.csv raw/${continent}_edges.csv
  ./osm4routing2pbbs
  #mv graph.adj graphs/${continent}_wgh.adj
  #mv coords.pbbs coordinates/$continent.pbbs
done
