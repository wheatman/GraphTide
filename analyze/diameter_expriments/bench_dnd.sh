#!/bin/bash
declare -a graph=(
  "africa"
  "asia"
  "australia-oceania"
  "central_america"
  "europe.shuffled"
  "north-america"
  "south-america"
  "wikipedia"
  "planet"
)


for g in "${graph[@]}"; do
  echo Running on ${g}.shuffled
  ./build//benchmark_dnd /ssd2/zhongqi/diameter_experiment/${g}.shuffled 
  echo
done

for g in "${graph[@]}"; do
  echo Running on ${g}.undirect
  ./build/benchmark_dnd /ssd2/zhongqi/diameter_experiment/${g}.undirect
  echo
done

