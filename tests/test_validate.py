from __future__ import annotations

import pandas as pd

from pybinding.convert import convert
from pybinding.validate import validate


def test_valid_signal_graph(tmp_path):
    path = tmp_path / "signal.bin"
    frame = pd.DataFrame(
        {
            "src": ["a", "b", "a"],
            "dst": ["b", "c", "c"],
            "timestamp": [1, 2, 3],
        }
    )
    convert(frame, path, signal=True, timestamp_type="uint32")

    report = validate(path, chunk_rows=2)

    assert report["valid"]
    assert report["statistics"]["rows_scanned"] == 3
    assert report["statistics"]["timestamp_order_violations"] == 0
    assert len(report["statistics"]["sha256"]) == 64


def test_unsorted_timestamps_are_rejected(tmp_path):
    path = tmp_path / "unsorted.bin"
    frame = pd.DataFrame(
        {
            "src": [0, 1],
            "dst": [1, 2],
            "timestamp": [2, 1],
        }
    )
    convert(frame, path, signal=True, timestamp_type="uint32")

    report = validate(path, chunk_rows=1)

    assert not report["valid"]
    assert report["statistics"]["timestamp_order_violations"] == 1


def test_update_sequence_check_detects_repeated_insert(tmp_path):
    path = tmp_path / "updates.bin"
    frame = pd.DataFrame(
        {
            "src": [0, 0],
            "dst": [1, 1],
            "timestamp": [1, 2],
            "info": ["insert", "insert"],
        }
    )
    convert(frame, path, signal=False, timestamp_type="uint32")

    shallow_report = validate(path)
    deep_report = validate(path, check_update_sequence=True)

    assert shallow_report["valid"]
    assert not deep_report["valid"]
    assert deep_report["statistics"]["repeated_insertions"] == 1


def test_file_size_mismatch_is_rejected(tmp_path):
    path = tmp_path / "trailing-byte.bin"
    frame = pd.DataFrame(
        {
            "src": [0],
            "dst": [1],
            "timestamp": [1],
        }
    )
    convert(frame, path, signal=True, timestamp_type="uint32")
    with path.open("ab") as stream:
        stream.write(b"x")

    report = validate(path)

    assert not report["valid"]
    assert any("File size mismatch" in error for error in report["errors"])

