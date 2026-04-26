#!/bin/bash

set -euo pipefail

root_dir="${DATA_DIR:-./dynamic_datasets}"
weighted_dir="${root_dir}/weighted"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

mkdir -p "${weighted_dir}/csv" "${weighted_dir}/binary_graphs"

continents=()
if [[ $# -gt 0 ]]; then
  continents=("$@")
else
  shopt -s nullglob
  for f in "${root_dir}/full_history/"*.osm.pbf; do
    name="$(basename "$f" .osm.pbf)"
    [[ "$name" == "history-latest" ]] && continue
    continents+=("$name")
  done
  shopt -u nullglob
fi

if [[ ${#continents[@]} -eq 0 ]]; then
  echo "No continents to process." >&2
  exit 1
fi

echo "Will process: ${continents[*]}"

for continent in "${continents[@]}"; do
  history="${root_dir}/full_history/${continent}.osm.pbf"
  csv="${weighted_dir}/csv/${continent}.csv"
  bin="${weighted_dir}/binary_graphs/${continent}.bin"

  if [[ ! -f "${history}" ]]; then
    echo "Skipping ${continent}: missing ${history}" >&2
    continue
  fi

  if [[ -f "${bin}" ]]; then
    echo "Skipping ${continent}: ${bin} already exists"
    continue
  fi

  echo "=== Processing ${continent} ==="
  echo "  Step 1/3: osm_extractor (pass 1: final state, pass 2: history) --weighted ..."
  "${script_dir}/osm_extractor" "${history}" --output "${csv}" --weighted

  echo "  Step 2/3: csv_to_bin ..."
  "${script_dir}/csv_to_bin" "${csv}" "${bin}"

  echo "  Step 3/3: cleaning up intermediate CSV ..."
  rm -f "${csv}"

  echo "  Done: ${bin}"
  ls -lh "${bin}"
  echo ""
done

echo "All done. Weighted datasets in ${weighted_dir}/binary_graphs/"
ls -lh "${weighted_dir}/binary_graphs/"
