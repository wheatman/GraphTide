
This directory contains the script to parse the <a href="https://api.semanticscholar.org/api-docs/datasets">Semantic Scholar</a> dataset.

## Download the raw dataset

Please make sure that the disk has at least 1.5 TB of free space.

```
python3 download.py --path <path_to_dataset>
```

This command can take several hours. Note that an `API_KEY` is required to download the dataset.

The raw dataset is stored in `.gz` files.
After downloading, please run `pigz -dk *.gz` to decompress the files.

## Parse the dataset

```
make
./parse --path <path_to_dataset>
```

The parsed dataset will be stored in the `<path_to_dataset>/semantic_scholar.csv`,
which is about 55GB in size.
The format is `source,dest,year`.
The vertex IDs are the semantic scholar corpus ids, not from 0 to N-1.
