#!/bin/bash

while read p; do
  echo "$p"
  NAME=$(echo "$p" | cut -d "-" -f 6 | cut -d "." -f 1)
  FNAME=$(echo "$p" | cut -d "/" -f 6)
  mkdir -p $NAME
  cd $NAME
  if test -f links.el; then
    echo "already done with $FNAME"
  else
    rm "$FNAME"
    wget "$p"
    LLsub "${WIKIPEDIA_SCRIPT_DIR:-.}/unzip_and_parse.sh" -- $FNAME
  fi
  cd -
done <files.txt