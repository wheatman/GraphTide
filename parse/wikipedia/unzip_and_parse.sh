#!/bin/bash
#SBATCH --exclusive


module load anaconda/2021a

FILE=$1
bunzip2 -f $FILE 
echo "done unzip"
python3 "${WIKIPEDIA_SCRIPT_DIR:-.}/xml_parse.py" $FILE
if [ $? -eq 0 ]; then
    echo "done parse"
    rm enwiki*
    mv links.el.temp links.el
else
    echo "Failed the parse"
fi
