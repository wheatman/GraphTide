import argparse
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict


def read_statistics_csv(path):
    return pd.read_csv(path, skipinitialspace=True)


def read_histogram_file(path):
    hist = defaultdict(int)
    with open(path, 'r') as f:
        for line in f:
            if not line.strip():
                continue
            size, count = map(int, line.strip().split(','))
            hist[size] += count
    return hist


def bucket_histogram_to_powers_of_two(hist):
    bucketed = defaultdict(int)
    for size, count in hist.items():
        bucket = 1 << (size.bit_length() - 1) if size > 0 else 0
        bucketed[bucket] += count
    return bucketed


def plot_csv_stats(stat_dfs, labels, outdir, prefix=""):
    os.makedirs(outdir, exist_ok=True)
    for column in stat_dfs[0].columns:
        if column in ('timestep', 'timestamp'):
            continue
        for log_scale in [False, True]:
            try:
                plt.figure()
                for df, label in zip(stat_dfs, labels):
                    plt.plot(df['timestep'], df[column], label=label)
                plt.title(f"{column} {'(log scale)' if log_scale else ''}")
                plt.xlabel("Timestep")
                plt.ylabel(column)
                if log_scale:
                    plt.yscale('log')
                plt.legend()
                plt.grid(True)
                suffix = "_log" if log_scale else ""
                filename = os.path.join(outdir, f"{prefix}{column}{suffix}.png")
                plt.savefig(filename)
                plt.close()
            except:
                print("issue with", filename)
                continue
def plot_csv_stats_timestamp(stat_dfs, labels, outdir, prefix=""):
    os.makedirs(outdir, exist_ok=True)
    min_max_timestamp = min(df["timestamp"].max() for df in stat_dfs)
    min_timestamp = min(df["timestamp"].min() for df in stat_dfs)
    for column in stat_dfs[0].columns:
        if column in ('timestep', 'timestamp'):
            continue
        for log_scale in [False, True]:
            try:
                plt.figure()
                for df, label in zip(stat_dfs, labels):
                    plt.plot(df['timestamp'], df[column], label=label)
                plt.title(f"{column} {'(log scale)' if log_scale else ''}")
                plt.xlabel("Timestamp")
                plt.ylabel(column)
                plt.xlim(right=min_max_timestamp, left=min_timestamp)
                if log_scale:
                    plt.yscale('log')
                plt.legend()
                plt.grid(True)
                suffix = "_log" if log_scale else ""
                filename = os.path.join(outdir, f"timestamp_{prefix}{column}{suffix}.png")
                plt.savefig(filename)
                plt.close()
            except:
                print("issue with", filename)
                continue



def plot_histograms(histograms, labels, outdir):
    if not histograms:
        print("No histograms to plot.")
        return

    os.makedirs(outdir, exist_ok=True)

    bucketed_all = [bucket_histogram_to_powers_of_two(h) for h in histograms]
    all_buckets = sorted(set(k for b in bucketed_all for k in b.keys()))
    x = np.arange(len(all_buckets))
    width = 0.8 / len(bucketed_all)

    for log_scale in [False, True]:
        plt.figure(figsize=(10, 6))
        for i, (label, bucketed) in enumerate(zip(labels, bucketed_all)):
            heights = [bucketed.get(k, 0) for k in all_buckets]
            plt.bar(x + i * width, heights, width=width, label=label)

        plt.xticks(x + width * (len(bucketed_all) - 1) / 2,
                   [str(b) for b in all_buckets], rotation=45)
        plt.xlabel("BFS Update Size (bucketed to power-of-two)")
        plt.ylabel("Count")
        if log_scale:
            plt.yscale("log")
            plt.title("BFS Update Size Histogram (log y-axis)")
            fname = "bfs_update_histogram_log.png"
        else:
            plt.title("BFS Update Size Histogram")
            fname = "bfs_update_histogram.png"
        plt.legend()
        plt.tight_layout()
        plt.savefig(os.path.join(outdir, fname))
        plt.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("dirs", nargs='+', help="Directories containing statistics.csv and optionally bfs_update_sizes_histogram")
    parser.add_argument("--outdir", default="plots", help="Output directory for plots")
    args = parser.parse_args()

    stat_dfs = []
    histograms = []
    labels_for_stats = []
    labels_for_hist = []

    static_stat_dfs = []
    labels_for_static_stats = []

    for d in args.dirs:
        stats_path = os.path.join(d, "statistics.csv")
        static_stats_path = os.path.join(d, "static.csv")
        hist_path = os.path.join(d, "bfs_update_sizes_histogram")
        label = os.path.basename(os.path.normpath(d))

        # Handle statistics.csv
        if os.path.exists(stats_path):
            try:
                stat_dfs.append(read_statistics_csv(stats_path))
                labels_for_stats.append(label)
            except Exception as e:
                print(f"Failed to read statistics.csv in {d}: {e}")
        else:
            print(f"Missing statistics.csv in {d}")

        # Handle statics.csv
        if os.path.exists(static_stats_path):
            try:
                static_stat_dfs.append(read_statistics_csv(static_stats_path))
                labels_for_static_stats.append(label)
            except Exception as e:
                print(f"Failed to read static.csv in {d}: {e}")
        else:
            print(f"Missing static.csv in {d}")

        # Handle histogram
        if os.path.exists(hist_path):
            try:
                histograms.append(read_histogram_file(hist_path))
                labels_for_hist.append(label)
            except Exception as e:
                print(f"Failed to read histogram in {d}: {e}")
        else:
            print(f"Missing bfs_update_sizes_histogram in {d}")

    if stat_dfs:
        plot_csv_stats(stat_dfs, labels_for_stats, args.outdir)
        plot_csv_stats_timestamp(stat_dfs, labels_for_stats, args.outdir)

    if static_stat_dfs:
        plot_csv_stats(static_stat_dfs, labels_for_static_stats, args.outdir, prefix="static_")
        plot_csv_stats_timestamp(static_stat_dfs, labels_for_static_stats, args.outdir, prefix="static_")

    if histograms:
        plot_histograms(histograms, labels_for_hist, args.outdir)

    print(f"Plots saved in '{args.outdir}'")
