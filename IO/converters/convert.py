#!python3
import os
import glob
import subprocess

input_dir = os.environ.get("INPUT_DIR", "./oag")
output_dir = os.environ.get("OUTPUT_DIR", "./oag_bin")

os.makedirs(output_dir, exist_ok=True)

edges_files = glob.glob(os.path.join(input_dir, "*.edges"))

for input_path in edges_files:
    base = os.path.basename(input_path)
    output_path = os.path.join(output_dir, base.replace(".edges", ".bin"))
    cmd = ["./convert_plain_signal", input_path, output_path]
    print(f"Running: {' '.join(cmd)}")
    subprocess.run(cmd, check=True)
