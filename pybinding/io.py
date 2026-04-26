"""Read and inspect DGDC binary graph files."""

from __future__ import annotations

import struct
from pathlib import Path
from typing import Any

import numpy as np

# ── DataType enum (must match IO/IO.hpp) ─────────────────────────────────────

_DTYPE_MAP: dict[int, tuple[str, np.dtype | None]] = {
    0:  ("NONE",    None),
    10: ("UINT8",   np.dtype("<u1")),
    11: ("UINT16",  np.dtype("<u2")),
    12: ("UINT32",  np.dtype("<u4")),
    13: ("UINT64",  np.dtype("<u8")),
    20: ("INT8",    np.dtype("<i1")),
    21: ("INT16",   np.dtype("<i2")),
    22: ("INT32",   np.dtype("<i4")),
    23: ("INT64",   np.dtype("<i8")),
    30: ("FLOAT32", np.dtype("<f4")),
    31: ("FLOAT64", np.dtype("<f8")),
}

_DTYPE_NAME_TO_CODE: dict[str, int] = {
    name.lower(): code for code, (name, _) in _DTYPE_MAP.items()
}

HEADER_SIZE = 32
_HEADER_FMT = "<QQQ5B3x"  # 3 uint64 + 5 uint8 + 3 padding bytes = 32 bytes


def _resolve_dtype(code: int) -> tuple[str, np.dtype | None]:
    if code not in _DTYPE_MAP:
        raise ValueError(f"Unknown DataType code: {code}")
    return _DTYPE_MAP[code]


def read_header(path: str | Path) -> dict[str, Any]:
    """Read the 32-byte header from a DGDC binary file.

    Returns a dict with keys: num_vertices, num_unique_edges, total_updates,
    signal, vertex_id_type, timestamp_type, directed, weight_type.
    """
    path = Path(path)
    with open(path, "rb") as f:
        raw = f.read(HEADER_SIZE)
    if len(raw) < HEADER_SIZE:
        raise ValueError(f"File too small for header ({len(raw)} bytes)")

    nv, ne, nu, signal, vid, ts, directed, wt = struct.unpack(_HEADER_FMT, raw)
    return {
        "num_vertices": nv,
        "num_unique_edges": ne,
        "total_updates": nu,
        "signal": bool(signal),
        "vertex_id_type": _resolve_dtype(vid)[0],
        "timestamp_type": _resolve_dtype(ts)[0],
        "directed": bool(directed),
        "weight_type": _resolve_dtype(wt)[0],
    }


def read_bin(path: str | Path) -> tuple[dict[str, Any], np.ndarray]:
    """Read a DGDC binary graph file.

    Returns (header_dict, structured_numpy_array).

    The numpy array has fields: src, dst, and conditionally timestamp, weight,
    info depending on the header.
    """
    path = Path(path)
    header = read_header(path)

    # Read raw codes from the fixed byte offsets in the 32-byte header:
    # [0:24] = 3x uint64, [24]=signal, [25]=vid, [26]=ts, [27]=directed, [28]=wt
    with open(path, "rb") as f:
        raw = f.read(HEADER_SIZE)
    vid_code = raw[25]
    ts_code = raw[26]
    wt_code = raw[28]

    _, vid_np = _resolve_dtype(vid_code)
    _, ts_np = _resolve_dtype(ts_code)
    _, wt_np = _resolve_dtype(wt_code)
    is_signal = header["signal"]

    # Build numpy structured dtype matching the binary row layout
    fields: list[tuple[str, np.dtype]] = []
    fields.append(("src", vid_np))
    fields.append(("dst", vid_np))
    if ts_np is not None:
        fields.append(("timestamp", ts_np))
    if wt_np is not None:
        fields.append(("weight", wt_np))
    if not is_signal:
        fields.append(("info", np.dtype("<u1")))  # UpdateInfo: 0=INSERT, 1=DELETE

    row_dtype = np.dtype(fields)
    total = header["total_updates"]

    data = np.fromfile(path, dtype=row_dtype, count=total, offset=HEADER_SIZE)
    return header, data
