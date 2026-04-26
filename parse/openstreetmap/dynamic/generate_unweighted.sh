#!/bin/bash

set -euo pipefail

root_dir="${DATA_DIR:-./dynamic_datasets}"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Two output directories: no_merge and merged
nomerge_dir="${root_dir}/unweighted/no_merge"
merged_dir="${root_dir}/unweighted/merged"

mkdir -p "${nomerge_dir}/csv" "${nomerge_dir}/binary_graphs"
mkdir -p "${merged_dir}/csv" "${merged_dir}/binary_graphs"

# Which mode to run: "no_merge", "merged", or "both"
mode="${1:-both}"
shift || true

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

echo "Mode: ${mode}, Continents: ${continents[*]}"

process() {
  local continent="$1"
  local out_dir="$2"
  local extra_flags="$3"
  local label="$4"

  local history="${root_dir}/full_history/${continent}.osm.pbf"
  local csv="${out_dir}/csv/${continent}.csv"
  local bin="${out_dir}/binary_graphs/${continent}.bin"

  if [[ ! -f "${history}" ]]; then
    echo "Skipping ${continent}: missing ${history}" >&2
    return
  fi

  if [[ -f "${bin}" ]]; then
    echo "Skipping ${continent} (${label}): ${bin} already exists"
    return
  fi

  echo "=== ${continent} (${label}) ==="
  echo "  osm_extractor ${extra_flags}..."
  "${script_dir}/osm_extractor" "${history}" --output "${csv}" ${extra_flags}

  echo "  csv_to_bin..."
  "${script_dir}/csv_to_bin" "${csv}" "${bin}"

  echo "  cleaning CSV..."
  rm -f "${csv}"

  echo "  Done: ${bin}"
  ls -lh "${bin}"
  echo ""
}

for continent in "${continents[@]}"; do
  if [[ "$mode" == "no_merge" || "$mode" == "both" ]]; then
    process "$continent" "$nomerge_dir" "" "no_merge"
  fi
  if [[ "$mode" == "merged" || "$mode" == "both" ]]; then
    process "$continent" "$merged_dir" "--merge-edges" "merged"
  fi
done

echo "=== Summary ==="
if [[ "$mode" == "no_merge" || "$mode" == "both" ]]; then
  echo "No-merge:"
  ls -lh "${nomerge_dir}/binary_graphs/" 2>/dev/null
fi
if [[ "$mode" == "merged" || "$mode" == "both" ]]; then
  echo "Merged:"
  ls -lh "${merged_dir}/binary_graphs/" 2>/dev/null
fi
