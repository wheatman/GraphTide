import os
import csv

# =========================
# Settings
# =========================

input_folder = "./"
output_file = "combined_results.csv"

# =========================
# Read all txt files
# =========================

data = {}

for filename in sorted(os.listdir(input_folder)):

    if not filename.lower().endswith(".txt"):
        continue

    file_path = os.path.join(input_folder, filename)

    with open(file_path, "r") as f:
        values = [
            line.strip()
            for line in f
            if line.strip()
        ]

    # Check that there are exactly 32 values
    if len(values) != 32:
        print(
            f"Warning: {filename} has {len(values)} values "
            f"instead of 32. Skipping."
        )
        continue

    # Remove .txt extension for column name
    column_name = os.path.splitext(filename)[0]

    data[column_name] = values


# =========================
# Write CSV
# =========================

column_names = list(data.keys())

with open(output_file, "w", newline="") as f:
    writer = csv.writer(f)

    # First row = column names
    writer.writerow(column_names)

    # Next 32 rows = values
    for i in range(32):
        row = [data[column][i] for column in column_names]
        writer.writerow(row)

print(f"Saved {len(column_names)} columns to {output_file}")