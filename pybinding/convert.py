"""Convert user-provided timestamped edge data to DGDC binary format."""

from __future__ import annotations

import struct
from pathlib import Path
from typing import Literal

import numpy as np
import pandas as pd

from pybinding.io import HEADER_SIZE, _DTYPE_NAME_TO_CODE, _DTYPE_MAP

# ── Public types ─────────────────────────────────────────────────────────────

DataTypeName = Literal[
    "uint8", "uint16", "uint32", "uint64",
    "int8", "int16", "int32", "int64",
    "float32", "float64", "none",
]


def _dtype_code(name: str) -> int:
    name = name.lower()
    if name not in _DTYPE_NAME_TO_CODE:
        raise ValueError(
            f"Unknown data type '{name}'. "
            f"Choices: {sorted(_DTYPE_NAME_TO_CODE.keys())}"
        )
    return _DTYPE_NAME_TO_CODE[name]


def _np_dtype_for(name: str) -> np.dtype | None:
    code = _dtype_code(name)
    return _DTYPE_MAP[code][1]


# ── Conversion ───────────────────────────────────────────────────────────────

def convert(
    input: str | Path | pd.DataFrame,
    output: str | Path,
    *,
    directed: bool = True,
    signal: bool = False,
    vertex_id_type: DataTypeName = "uint32",
    timestamp_type: DataTypeName = "uint32",
    weight_type: DataTypeName = "none",
    src_col: str = "src",
    dst_col: str = "dst",
    timestamp_col: str = "timestamp",
    weight_col: str = "weight",
    info_col: str = "info",
    sep: str | None = None,
) -> dict:
    """Convert a DataFrame or text file of edges to DGDC binary format.

    Parameters
    ----------
    input : path or DataFrame
        If a path, reads a delimited text file. Expected columns are
        determined by *_col parameters.  If a DataFrame, uses it directly.
    output : path
        Destination binary file.
    directed : bool
        Whether edges are directed.
    signal : bool
        If True, treat as a signal graph (insert-only, no info column).
    vertex_id_type, timestamp_type, weight_type : DataTypeName
        Binary types for each field.  Use "none" to omit timestamp or weight.
    src_col, dst_col, timestamp_col, weight_col, info_col : str
        Column names when reading from a DataFrame or text file.
    sep : str or None
        Delimiter for text files. None = whitespace.

    Returns
    -------
    dict
        The header that was written (same schema as ``read_header``).
    """
    output = Path(output)

    # ── Load data ────────────────────────────────────────────────────────
    if isinstance(input, (str, Path)):
        input = Path(input)
        # Detect columns present
        cols_needed = [src_col, dst_col]
        if timestamp_type != "none":
            cols_needed.append(timestamp_col)
        if weight_type != "none":
            cols_needed.append(weight_col)
        if not signal:
            cols_needed.append(info_col)

        df = pd.read_csv(
            input,
            sep=sep,
            header=0 if _file_has_header(input, sep) else None,
            names=None,
            engine="c",
        )
        # If no header was detected, assign positional names
        if isinstance(df.columns[0], int):
            positional = [src_col, dst_col]
            if timestamp_type != "none":
                positional.append(timestamp_col)
            if weight_type != "none":
                positional.append(weight_col)
            if not signal:
                positional.append(info_col)
            df.columns = positional[: len(df.columns)]
    else:
        df = input

    # ── Remap vertex IDs to contiguous range ─────────────────────────────
    all_ids = pd.concat([df[src_col], df[dst_col]], ignore_index=True)
    uniques = all_ids.unique()
    mapping = {v: i for i, v in enumerate(uniques)}
    src = df[src_col].map(mapping).values
    dst = df[dst_col].map(mapping).values

    num_vertices = len(uniques)
    unique_edges = len(set(zip(src, dst)))
    total_updates = len(df)

    # ── Numpy arrays with correct dtypes ─────────────────────────────────
    vid_np = _np_dtype_for(vertex_id_type)
    src = src.astype(vid_np)
    dst = dst.astype(vid_np)

    ts_np = _np_dtype_for(timestamp_type)
    timestamps = None
    if ts_np is not None and timestamp_col in df.columns:
        timestamps = df[timestamp_col].values.astype(ts_np)

    wt_np = _np_dtype_for(weight_type)
    weights = None
    if wt_np is not None and weight_col in df.columns:
        weights = df[weight_col].values.astype(wt_np)

    info = None
    if not signal:
        if info_col in df.columns:
            raw_info = df[info_col].astype(str).str.lower()
            info = np.where(raw_info.isin(["1", "delete", "del"]), 1, 0).astype(
                np.uint8
            )
        else:
            info = np.zeros(total_updates, dtype=np.uint8)

    # ── Write binary ─────────────────────────────────────────────────────
    header_bytes = struct.pack(
        "<QQQ5B3x",
        num_vertices,
        unique_edges,
        total_updates,
        int(signal),
        _dtype_code(vertex_id_type),
        _dtype_code(timestamp_type),
        int(directed),
        _dtype_code(weight_type),
    )
    assert len(header_bytes) == HEADER_SIZE

    with open(output, "wb") as f:
        f.write(header_bytes)
        for i in range(total_updates):
            f.write(src[i].tobytes())
            f.write(dst[i].tobytes())
            if timestamps is not None:
                f.write(timestamps[i].tobytes())
            if weights is not None:
                f.write(weights[i].tobytes())
            if info is not None:
                f.write(info[i].tobytes())

    header_dict = {
        "num_vertices": num_vertices,
        "num_unique_edges": unique_edges,
        "total_updates": total_updates,
        "signal": signal,
        "vertex_id_type": vertex_id_type.upper(),
        "timestamp_type": timestamp_type.upper(),
        "directed": directed,
        "weight_type": weight_type.upper(),
    }
    return header_dict


def _file_has_header(path: Path, sep: str | None) -> bool:
    """Heuristic: if the first field of the first line is non-numeric, assume header."""
    with open(path) as f:
        first_line = f.readline().strip()
    if not first_line:
        return False
    delim = sep if sep else None
    first_field = first_line.split(delim)[0].strip()
    try:
        float(first_field)
        return False
    except ValueError:
        return True
