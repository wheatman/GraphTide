# GraphTide
This repo is to aid in the collection and analysis of dynamic graph datasets.

It is broken into a few different subdirectories.


## Datasets
Our datasets are publicly available at https://huggingface.co/datasets/DynamicGraphsProvider/GraphTide.

Shuffled graphs and real dynamic graphs exhibit substantial differences across many graph properties. The classification of the measured properties by graph category is summarized in the [supplementary material](supplementary.pdf).

## Contributing a Dataset

We welcome community-contributed real-world dynamic graphs. A source parser is
encouraged but is not required.

The contribution process has two stages:

1. Open a **New dataset proposal** issue in this GitHub repository with the
   source, license, graph semantics, timestamp information, cleaning decisions,
   and approximate size.
2. After approval, validate the binary graph and upload it to the GraphTide
   Hugging Face dataset as a pull request.

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for the complete submission and
cleaning guidelines.

Before submitting a binary file, run:

```bash
python -m pybinding.validate /path/to/dataset.bin
```

For an update graph, the optional stateful check also verifies that insertions
and deletions form a legal sequence:

```bash
python -m pybinding.validate /path/to/dataset.bin --check-update-sequence
```

## Dependencies Setup
This repo expects shared third-party dependencies under `external/`.

After cloning, run from the repo root:

```bash
git submodule sync --recursive
git submodule update --init --recursive
```


## analyze
This directory helps to analyze a graph once it has been parsed. It is designed to run on a dynamic graph and determine statistics of the graph.

For build and run instructions, see `analyze/README.md`.

## parse
This directory contains code to help download and parse different dynamic graphs.

It is broken up by data source.

## Graph Types
This repo contains dynamic graphs of two different types.  

### Signal Graphs 
The first, and more common, is what we will call a signal graph.  These graphs each edge is a signal of a connection.  For example, consider a graph of emails.  Each email tells a connection between the sender and receiver, but the email is only an instant in time.  In these graphs their are no edge removals, and how long an edge should last is left up to the user and so removals are not in the underlying datat.

### Update Graphs
These are graphs of actual things. Where the graph is an exact graph of something and edges can be both added and removed explicitly.  For example in a road graph, roads will be opened and closed at exact times.

## File Format

A few different file formats are used. These are described in detail in `IO/README.md`.

### Signal Graphs 

The basic files are stored as *.edges This will be a white space separated file with each row simply contained source, dest, and a timestamp.  If the file is *weighted.edges then each row also has a 4th field which is the weight.

Some large files are stored with a .bin, in which the three fields are replaced with the binary form with each of the source, dest and timestamp being replaced by 5 bytes.  These can be much smaller and much faster to parse in parallel.


### Update Graphs
Update graphs need an additional field which specifies if it's an addition or a deletion.  This field is at the beginning of the elements about each edge.

## FAQ

### Were the released datasets cleaned?

Yes. For the datasets currently released in GraphTide, malformed
records and self-loops were removed. Repeated signals and parallel
edges were retained when they represented distinct real-world events.
Other processing decisions are source-specific; the available parsing
code and documentation can be found under [`parse/`](parse/).

### Can I change the timestamp unit or granularity?

Yes. The canonical datasets preserve their original timestamps, but
users can load a graph, transform its timestamp column, and convert it
back to the GraphTide binary format. For example, a monotone rescaling
can change timestamp units, while temporal binning can produce a
coarser time granularity.

### Can I introduce missing, noisy, or out-of-order events?

Yes, by modifying the input DataFrame or CSV before calling
`convert`. Events can be dropped, timestamps can be jittered, or rows
can be reordered. GraphTide does not prescribe a standard noise model,
because the appropriate transformation is application-dependent.
Modified traces should be identified as derived data rather than
replacements for the canonical real-order datasets.

### Do I need to provide a parser when contributing a dataset?

No. Providing a parser is encouraged but optional. Contributors can
submit a documented binary graph through the process described in
[`CONTRIBUTING.md`](CONTRIBUTING.md).
