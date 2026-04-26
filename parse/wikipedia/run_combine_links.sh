#!/bin/bash
#SBATCH --exclusive


module load anaconda/2021a

python3 "${WIKIPEDIA_SCRIPT_DIR:-.}/combine_links.py"  */links.el