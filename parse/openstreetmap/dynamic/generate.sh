#!/bin/bash

set -euo pipefail

root_dir="${DATA_DIR:-./dynamic_datasets}"

mkdir -p "${root_dir}/poly" "${root_dir}/full_history" "${root_dir}/csv" "${root_dir}/binary_graphs"

poly_files=()
if [[ $# -gt 0 ]]; then
  for region in "$@"; do
    [[ "$region" == "planet" ]] && continue
    candidate="${root_dir}/poly/${region}.poly"
    if [[ -f "${candidate}" ]]; then
      poly_files+=("${candidate}")
    else
      echo "Skipping ${region}: missing ${candidate}" >&2
    fi
  done
else
  shopt -s nullglob
  poly_files=("${root_dir}/poly/"*.poly)
  shopt -u nullglob
fi

for poly_path in "${poly_files[@]}"; do
  continent="$(basename "${poly_path}" .poly)"
  history_out="${root_dir}/full_history/${continent}.osm.pbf"
  csv_out="${root_dir}/csv/${continent}.csv"
  bin_out="${root_dir}/binary_graphs/${continent}.bin"

  if [[ ! -f "${history_out}" ]]; then
    echo "Extracting ${continent} from history-latest.osm.pbf..."
    osmium extract -p "${poly_path}" --with-history -s complete_ways -o "${history_out}" "${root_dir}/full_history/history-latest.osm.pbf"
  fi
  ./osm_extractor "${history_out}" --output "${csv_out}"
  ./csv_to_bin "${csv_out}" "${bin_out}"
done

# Planet: run directly on the full history file (no poly extraction needed)
if [[ $# -eq 0 ]] || [[ " $* " == *" planet "* ]]; then
  planet_history="${root_dir}/full_history/history-latest.osm.pbf"
  planet_csv="${root_dir}/csv/planet.csv"
  planet_bin="${root_dir}/binary_graphs/planet.bin"

  if [[ -f "${planet_history}" ]]; then
    echo "=== Processing planet ==="
    ./osm_extractor "${planet_history}" --output "${planet_csv}"
    ./csv_to_bin "${planet_csv}" "${planet_bin}"
  else
    echo "Skipping planet: missing ${planet_history}" >&2
  fi
fi
