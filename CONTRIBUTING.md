# Contributing a Dataset to GraphTide

GraphTide welcomes real-world dynamic graph datasets with authentic temporal
information. Contributions are data-first: you do **not** need to provide the
code that originally collected or parsed the data. A parser is welcome when it
is available, but it is optional.

Large binary files are submitted to the
[GraphTide Hugging Face dataset](https://huggingface.co/datasets/DynamicGraphsProvider/GraphTide).
GitHub is used for the proposal, documentation, validation tools, and review
coordination.

## Contribution Workflow

1. Open a **New dataset proposal** issue in this repository. Do this before
   uploading a large file so the maintainers can review the source, license,
   graph semantics, and intended filename.
2. After the proposal is approved, prepare the graph in the GraphTide binary
   format described in [`IO/README.md`](IO/README.md).
3. Run the validator and resolve every reported error:

   ```bash
   python -m pybinding.validate /path/to/dataset.bin
   ```

   For an update graph, also run the stateful update-sequence check when the
   graph fits in available memory:

   ```bash
   python -m pybinding.validate /path/to/dataset.bin \
     --check-update-sequence
   ```

4. Upload the `.bin` file directly to the GraphTide Hugging Face repository as
   a pull request. Use `signal_graphs/<dataset-id>.bin` for a signal graph and
   `update_graphs/<dataset-id>.bin` for an update graph.
5. Link the Hugging Face pull request from the original GitHub proposal issue.
   The maintainers will rerun validation, review the documented cleaning
   choices, merge the data, and add it to the public dataset catalog.

An external contributor without write access can create the Hugging Face pull
request with:

```bash
pip install -U huggingface_hub hf_xet
hf auth login
hf upload DynamicGraphsProvider/GraphTide \
  /path/to/dataset.bin signal_graphs/<dataset-id>.bin \
  --repo-type dataset \
  --create-pr \
  --commit-message "Add <dataset-id> dynamic graph"
```

Replace `signal_graphs` with `update_graphs` when appropriate. Hugging Face
recommends splitting individual files larger than 200 GB before upload.

## Required Dataset Description

The proposal issue is the permanent record of the contribution. It must state:

- the dataset name and proposed short identifier;
- the public source URL, source version or snapshot date, citation, and license;
- whether the dataset is a signal graph or an update graph;
- whether it is directed and/or weighted;
- what vertices represent;
- what each edge signal or update represents;
- the timestamp unit and covered time range;
- all cleaning and transformation decisions; and
- approximate numbers of vertices, unique edges, and temporal events.

Do not include confidential data, redistributed user-generated content, or
raw identifier mappings that expose people or accounts. The contributor is
responsible for confirming that the dataset can be redistributed under its
source terms.

## Cleaning and Representation Guidelines

GraphTide does not impose a blanket rule that removes self-loops, repeated
signals, or parallel edges. These records can be meaningful in temporal data.
Instead, contributors must preserve the source semantics and document their
decisions.

### Invalid records

Remove records that cannot be interpreted, such as rows with missing
endpoints, malformed operation types, or timestamps outside the documented
domain. Report how many records were removed and why.

### Repeated events and parallel edges

Repeated signals between the same endpoints are normally preserved. An exact
duplicate should be removed only when there is evidence that it is a collection
or processing artifact. Document any deduplication rule and the number of
records it removes.

### Self-loops

Self-loops may be preserved when they have a real interpretation and removed
otherwise. State the chosen policy and the number of affected records.

### Timestamps and ordering

- Preserve authentic source timestamps in the canonical release.
- Store events in nondecreasing timestamp order.
- Preserve source order for equal timestamps when it is available; otherwise
  use a deterministic tie-breaking rule and document it.
- State the timestamp unit explicitly.
- Retimestamped, shuffled, jittered, or sampled traces must be labeled as
  derived data and must not replace the canonical real-order dataset.

### Vertex identifiers

Map vertex identifiers to a contiguous integer range starting at zero. The
mapping must be deterministic for a fixed input. Do not publish the original
identifier mapping when it contains sensitive or restricted identifiers.

### Update graphs

An update graph is replayed from an empty graph. For every edge, the first
operation must be an insertion, and subsequent operations must alternate
between deletion and insertion. A deletion of an absent edge or repeated
insertion of a present edge is invalid for the simple-graph representation and
must be resolved and reported before submission.

## What Validation Checks

The standard validator streams the file and checks:

- the 32-byte header and supported data types;
- the expected file size and record count;
- source and destination vertex-ID ranges;
- nondecreasing timestamp order;
- legal update-type byte values;
- counts of insertions, deletions, and self-loops; and
- a SHA-256 checksum for the submitted artifact.

The optional update-sequence check additionally replays an update graph and
detects repeated insertions and deletions of absent edges. Its memory use is
proportional to the number of active edges.

Warnings do not automatically reject a dataset, but they must be explained in
the proposal. Validation errors must be fixed before the Hugging Face pull
request is merged.

## Maintenance and Revisions

Accepted data-only contributions are maintained as versioned snapshots. A
future correction or refresh must open a new proposal, explain the change, and
submit a new Hugging Face revision; existing revisions remain accessible in
repository history. Contributions with an optional parser may be refreshed
from their upstream source, but GraphTide does not require or promise a refresh
schedule.

