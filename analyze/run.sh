#!/bin/bash

# Directory containing the .bin files
input_dir=~/storage/binary_graphs
output_dir=~/storage/scratch6
plot_dir=~/storage/plots6

MAX_JOBS=8
PARLAY_NUM_THREADS=24
mkdir -p $output_dir
mkdir -p $plot_dir


PRINT_FREQ=1000
MAX_UPDATES=-1
STATIC_COUNT=1000
STATIC_ALGS="--static_algorithm_count $STATIC_COUNT"
SRC=1

process_file() {
    file="$1"
    filename=$(basename -- "$file")
    dataset="${filename%.bin}"

    if [[ "$filename" == *wikipedia* ]] || [[ "$filename" == *reddit* ]]; then
        echo "Skipping dataset: $dataset"
        return
    fi

    # if [[ "$filename" == *bluesky* ]] || [[ "$filename" == *btc_stream* ]] || [[ "$filename" == *copresence-Thiers13* ]] || [[ "$filename" == *ia-online-ads-criteo* ]] || [[ "$filename" == *linux* ]] || [[ "$filename" == *llvm* ]] || [[ "$filename" == *rec-amazon* ]] || [[ "$filename" == *rec-amz-Books* ]] || [[ "$filename" == *rec-amz-CDs* ]] || [[ "$filename" == *rec-amz-Cell* ]] || [[ "$filename" == *rec-amz-Clothing* ]]; then
    #     echo "non window only: $dataset"
    #     echo "Processing dataset: $dataset"
    #     mkdir -p $output_dir/${dataset}_true
    #     # Run the commands
    #     ./run --output "$output_dir/${dataset}_true" --input "$file" --print_freq $PRINT_FREQ --max_updates $MAX_UPDATES $STATIC_ALGS --src $SRC > $output_dir/${dataset}_true/dump
    #     mkdir -p $output_dir/${dataset}_shuffled
    #     ./run --output "$output_dir/${dataset}_shuffled" --input "$file" --edges_shuffle --print_freq $PRINT_FREQ --max_updates $MAX_UPDATES $STATIC_ALGS --src $SRC > $output_dir/${dataset}_shuffled/dump
    #     python3 python_helpers/plot.py "$output_dir/${dataset}_true" "$output_dir/${dataset}_shuffled" --outdir "$plot_dir/$dataset"
    #     continue
    # fi

    partial_skip=false
    # if [[ "$filename" == *bluesky* ]]; then
    #     partial_skip=true
    #     echo "Detected partial dataset — skipping pre-window section and first 4 window sizes."
    # fi

    if [[ "$partial_skip" == false ]]; then
        echo "Processing dataset: $dataset"
        mkdir -p $output_dir/${dataset}_true
        # Run the commands
        PARLAY_NUM_THREADS=$PARLAY_NUM_THREADS ./run --output "$output_dir/${dataset}_true" --input "$file" --print_freq $PRINT_FREQ --max_updates $MAX_UPDATES $STATIC_ALGS --src $SRC > $output_dir/${dataset}_true/dump
        mkdir -p $output_dir/${dataset}_shuffled
        PARLAY_NUM_THREADS=$PARLAY_NUM_THREADS ./run --output "$output_dir/${dataset}_shuffled" --input "$file" --edges_shuffle --print_freq $PRINT_FREQ --max_updates $MAX_UPDATES $STATIC_ALGS --src $SRC > $output_dir/${dataset}_shuffled/dump
        python3 python_helpers/plot.py "$output_dir/${dataset}_true" "$output_dir/${dataset}_shuffled" --outdir "$plot_dir/$dataset"
    fi

    all_window_sizes=(10 100 1000 10000 100000 1000000 10000000)

    # If partial_skip, skip the first 5 window sizes
    if [[ "$partial_skip" == true ]]; then
        window_sizes=("${all_window_sizes[@]:5}")  # skip 10,100,1000,10000,100000
    else
        window_sizes=("${all_window_sizes[@]}")
    fi

    for window_size in "${window_sizes[@]}"; do
        echo "Running window_size=$window_size for $dataset"
        mkdir -p $output_dir/${dataset}_window_${window_size}
        # Run the commands
        PARLAY_NUM_THREADS=$PARLAY_NUM_THREADS ./run --output "$output_dir/${dataset}_window_${window_size}" --input "$file" --print_freq $PRINT_FREQ --max_updates $MAX_UPDATES $STATIC_ALGS --src $SRC --window_size $window_size > $output_dir/${dataset}_window_${window_size}/dump
                
        mkdir -p $output_dir/${dataset}_shuffled_window_${window_size}
        # Run the commands
        PARLAY_NUM_THREADS=$PARLAY_NUM_THREADS ./run --output "$output_dir/${dataset}_shuffled_window_${window_size}" --input "$file" --edges_shuffle --print_freq $PRINT_FREQ --max_updates $MAX_UPDATES $STATIC_ALGS --src $SRC --window_size $window_size > $output_dir/${dataset}_shuffled_window_${window_size}/dump

        python3 python_helpers/plot.py $output_dir/${dataset}_window_${window_size} $output_dir/${dataset}_shuffled_window_${window_size} --outdir "$plot_dir/${dataset}_each_window_"${window_size}
    done
    python3 python_helpers/plot.py $output_dir/${dataset}_window_* --outdir "$plot_dir/${dataset}_window"
}

process_update_file() {
    file="$1"
    filename=$(basename -- "$file")
    dataset="${filename%.bin}"


    echo "Processing dataset: $dataset"
    mkdir -p $output_dir/${dataset}_true
    # Run the commands
    PARLAY_NUM_THREADS=$PARLAY_NUM_THREADS ./run --output "$output_dir/${dataset}_true" --input "$file" --print_freq $PRINT_FREQ --max_updates $MAX_UPDATES $STATIC_ALGS --src $SRC > $output_dir/${dataset}_true/dump
    mkdir -p $output_dir/${dataset}_shuffled
    PARLAY_NUM_THREADS=$PARLAY_NUM_THREADS ./run --output "$output_dir/${dataset}_shuffled" --input "$file" --edges_shuffle --print_freq $PRINT_FREQ --max_updates $MAX_UPDATES $STATIC_ALGS --src $SRC > $output_dir/${dataset}_shuffled/dump
    python3 python_helpers/plot.py "$output_dir/${dataset}_true" "$output_dir/${dataset}_shuffled" --outdir "$plot_dir/$dataset"
}


# Loop over all .bin files in the input directory
# for file in "$input_dir"/*.bin; do
#     process_file "$file" &   # run in background
#     ((running++))
#     if ((running >= MAX_JOBS)); then
#         wait -n  # wait for one job to finish before starting another
#         ((running--))
#     fi
# done

process_file ~/storage/binary_graphs/llvm.bin
# process_update_file ~/storage/binary_graphs/wikipedia.bin
