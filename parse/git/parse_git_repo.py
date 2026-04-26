import subprocess

from datetime import datetime
import time
import sys
import csv


def get_git_commits(repo_path):
    try:
        # Run git log to get commit details
        log_output = subprocess.check_output(
            [
                "git", "log", "--pretty=format:%H|%an|%ad", "--name-only", "--date=iso"  # ISO 8601 timestamp
            ],
            cwd=repo_path
        )
    except subprocess.CalledProcessError as e:
        print(f"Error running git log: {e}")
        return []
    
    commits = []
    ids = {}
    current_id = 0

    past_first = False
    files = []
    edges = []
    for line in log_output.splitlines():
        if b'|' in line:
            if past_first:
                if author not in ids:
                    ids[author] = current_id
                    current_id+=1
                for f in files:
                    if f not in ids:
                        ids[f] = current_id
                        current_id += 1
                    edges.append((ids[author], ids[f],timestamp))
            past_first = True
            files = []
            commit_hash, author, timestamp_str = line.split(b'|', 2)
            timestamp_str = timestamp_str.decode("utf-8")
            dt = datetime.strptime(timestamp_str, "%Y-%m-%d %H:%M:%S %z")
            timestamp = int(dt.timestamp())
        elif line.strip():
            files.append(line.strip())

    if author not in ids:
        ids[author] = current_id
        current_id+=1
    for f in files:
        if f not in ids:
            ids[f] = current_id
            current_id += 1
        edges.append((ids[author], ids[f],timestamp))
    print("num vertices = ", len(ids))
    print("num updates = ", len(edges))
    return edges

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py <path_to_git_repo> <output file>")
        sys.exit(1)
    
    repo_path = sys.argv[1]
    output_file = sys.argv[2]
    edges = get_git_commits(repo_path)
    with open(output_file, mode="w") as file:
        writer = csv.writer(file, delimiter="\t")  # Use tab as a delimiter
        writer.writerows(edges)  # Write all rows

