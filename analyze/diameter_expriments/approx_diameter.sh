#!/bin/bash
declare -a graph=(
  "africa"
  "asia"
  "australia-oceania"
  "central_america"
  "europe"
  "north-america"
  "south-america"
  "wikipedia"
  "planet"
)


for g in "${graph[@]}"; do
  echo Running on ${g}.shuffled
  ./build/approx_diameter /ssd2/zhongqi/diameter_experiment/${g}.shuffled 32 > diameter_results/${g}_shuffled.txt
  echo
done

for g in "${graph[@]}"; do
  echo Running on ${g}.undirect
  ./build/approx_diameter /ssd2/zhongqi/diameter_experiment/${g}.undirect 32 > diameter_results/${g}_undirect.txt
  echo
done

