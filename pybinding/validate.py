"""Validate a GraphTide binary graph without loading it fully into memory."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path
from typing import Any

import numpy as np

from pybinding.io import HEADER_SIZE, _DTYPE_MAP


_HEADER_FORMAT = "<QQQ5B3x"
_SUPPORTED_CODES = set(_DTYPE_MAP)


def _dtype_name(code: int) -> str:
    entry = _DTYPE_MAP.get(code)
    return entry[0] if entry else f"UNKNOWN({code})"


def _row_dtype(
    vertex_id_code: int,
    timestamp_code: int,
    weight_code: int,
    signal: bool,
) -> np.dtype | None:
    if vertex_id_code not in _DTYPE_MAP or vertex_id_code == 0:
        return None
    if timestamp_code not in _DTYPE_MAP or weight_code not in _DTYPE_MAP:
        return None

    fields: list[tuple[str, np.dtype]] = [
        ("src", _DTYPE_MAP[vertex_id_code][1]),
        ("dst", _DTYPE_MAP[vertex_id_code][1]),
    ]
    if timestamp_code != 0:
        fields.append(("timestamp", _DTYPE_MAP[timestamp_code][1]))
    if weight_code != 0:
        fields.append(("weight", _DTYPE_MAP[weight_code][1]))
    if not signal:
        fields.append(("info", np.dtype("<u1")))
    return np.dtype(fields)


def validate(
    path: str | Path,
    *,
    check_update_sequence: bool = False,
    chunk_rows: int = 1_000_000,
) -> dict[str, Any]:
    """Validate a GraphTide binary graph and return a JSON-serializable report.

    The standard checks use constant memory. ``check_update_sequence`` replays
    an update graph and therefore uses memory proportional to its active edges.
    """
    path = Path(path)
    errors: list[str] = []
    warnings: list[str] = []
    stats: dict[str, Any] = {
        "rows_scanned": 0,
        "self_loops": 0,
        "min_vertex_id": None,
        "max_vertex_id": None,
        "min_timestamp": None,
        "max_timestamp": None,
    }
    report: dict[str, Any] = {
        "path": str(path.resolve()),
        "valid": False,
        "errors": errors,
        "warnings": warnings,
        "header": None,
        "statistics": stats,
    }

    if chunk_rows <= 0:
        raise ValueError("chunk_rows must be positive")
    if not path.is_file():
        errors.append(f"File not found: {path}")
        return report

    file_size = path.stat().st_size
    stats["file_size_bytes"] = file_size

    with path.open("rb") as stream:
        raw_header = stream.read(HEADER_SIZE)
        if len(raw_header) != HEADER_SIZE:
            errors.append(
                f"File is {file_size} bytes; a GraphTide header requires "
                f"{HEADER_SIZE} bytes"
            )
            return report

        (
            num_vertices,
            num_unique_edges,
            total_updates,
            signal_raw,
            vertex_id_code,
            timestamp_code,
            directed_raw,
            weight_code,
        ) = struct.unpack(_HEADER_FORMAT, raw_header)

        header = {
            "num_vertices": num_vertices,
            "num_unique_edges": num_unique_edges,
            "total_updates": total_updates,
            "signal": bool(signal_raw),
            "vertex_id_type": _dtype_name(vertex_id_code),
            "timestamp_type": _dtype_name(timestamp_code),
            "directed": bool(directed_raw),
            "weight_type": _dtype_name(weight_code),
        }
        report["header"] = header

        if signal_raw not in (0, 1):
            errors.append(f"Signal flag must be 0 or 1, found {signal_raw}")
        if directed_raw not in (0, 1):
            errors.append(f"Directed flag must be 0 or 1, found {directed_raw}")
        if vertex_id_code not in _SUPPORTED_CODES or vertex_id_code == 0:
            errors.append(
                "vertex_id_type must be a supported numeric type other than NONE, "
                f"found {_dtype_name(vertex_id_code)}"
            )
        if timestamp_code not in _SUPPORTED_CODES:
            errors.append(
                f"Unsupported timestamp type code: {_dtype_name(timestamp_code)}"
            )
        if weight_code not in _SUPPORTED_CODES:
            errors.append(f"Unsupported weight type code: {_dtype_name(weight_code)}")
        if any(raw_header[29:32]):
            warnings.append("The three reserved header bytes are not zero")
        if num_unique_edges > total_updates:
            errors.append(
                "Header num_unique_edges exceeds total_updates: "
                f"{num_unique_edges} > {total_updates}"
            )

        dtype = _row_dtype(
            vertex_id_code,
            timestamp_code,
            weight_code,
            bool(signal_raw),
        )
        if dtype is None or dtype.itemsize == 0:
            return report

        row_size = dtype.itemsize
        expected_size = HEADER_SIZE + total_updates * row_size
        stats["row_size_bytes"] = row_size
        stats["expected_file_size_bytes"] = expected_size
        if file_size != expected_size:
            errors.append(
                f"File size mismatch: expected {expected_size} bytes from the "
                f"header, found {file_size}"
            )

        hasher = hashlib.sha256()
        hasher.update(raw_header)

        previous_timestamp: int | float | None = None
        timestamp_order_violations = 0
        invalid_timestamp_values = 0
        invalid_info_values = 0
        out_of_range_vertex_ids = 0
        negative_vertex_ids = 0
        insertions = 0
        deletions = 0
        active_edges: set[tuple[int, int]] | None = None
        repeated_insertions = 0
        deletions_of_absent_edges = 0

        if check_update_sequence and bool(signal_raw):
            warnings.append(
                "--check-update-sequence was requested for a signal graph and was ignored"
            )
        elif check_update_sequence:
            active_edges = set()

        bytes_per_chunk = row_size * chunk_rows
        complete_rows_seen = 0
        while True:
            block = stream.read(bytes_per_chunk)
            if not block:
                break
            hasher.update(block)

            complete_bytes = len(block) - (len(block) % row_size)
            if complete_bytes == 0:
                continue
            rows = np.frombuffer(block[:complete_bytes], dtype=dtype)

            rows_left = max(0, total_updates - complete_rows_seen)
            if len(rows) > rows_left:
                rows = rows[:rows_left]
            complete_rows_seen += len(rows)
            if len(rows) == 0:
                continue

            src = rows["src"]
            dst = rows["dst"]
            stats["rows_scanned"] += int(len(rows))
            stats["self_loops"] += int(np.count_nonzero(src == dst))

            src_min = int(src.min())
            dst_min = int(dst.min())
            src_max = int(src.max())
            dst_max = int(dst.max())
            chunk_min_id = min(src_min, dst_min)
            chunk_max_id = max(src_max, dst_max)
            if stats["min_vertex_id"] is None:
                stats["min_vertex_id"] = chunk_min_id
                stats["max_vertex_id"] = chunk_max_id
            else:
                stats["min_vertex_id"] = min(stats["min_vertex_id"], chunk_min_id)
                stats["max_vertex_id"] = max(stats["max_vertex_id"], chunk_max_id)

            if np.issubdtype(src.dtype, np.signedinteger):
                negative_vertex_ids += int(np.count_nonzero((src < 0) | (dst < 0)))
            out_of_range_vertex_ids += int(
                np.count_nonzero((src >= num_vertices) | (dst >= num_vertices))
            )

            if "timestamp" in dtype.names:
                timestamps = rows["timestamp"]
                if np.issubdtype(timestamps.dtype, np.floating):
                    invalid_timestamp_values += int(
                        np.count_nonzero(~np.isfinite(timestamps))
                    )
                    finite = timestamps[np.isfinite(timestamps)]
                else:
                    finite = timestamps

                if len(finite):
                    chunk_min_ts = finite.min().item()
                    chunk_max_ts = finite.max().item()
                    if stats["min_timestamp"] is None:
                        stats["min_timestamp"] = chunk_min_ts
                        stats["max_timestamp"] = chunk_max_ts
                    else:
                        stats["min_timestamp"] = min(
                            stats["min_timestamp"], chunk_min_ts
                        )
                        stats["max_timestamp"] = max(
                            stats["max_timestamp"], chunk_max_ts
                        )

                if previous_timestamp is not None:
                    timestamp_order_violations += int(timestamps[0] < previous_timestamp)
                if len(timestamps) > 1:
                    timestamp_order_violations += int(
                        np.count_nonzero(timestamps[1:] < timestamps[:-1])
                    )
                previous_timestamp = timestamps[-1].item()

            if "info" in dtype.names:
                info = rows["info"]
                valid_info = (info == 0) | (info == 1)
                invalid_info_values += int(np.count_nonzero(~valid_info))
                insertions += int(np.count_nonzero(info == 0))
                deletions += int(np.count_nonzero(info == 1))

                if active_edges is not None:
                    directed = bool(directed_raw)
                    for source, destination, operation in zip(src, dst, info):
                        source_id = int(source)
                        destination_id = int(destination)
                        if directed or source_id <= destination_id:
                            edge = (source_id, destination_id)
                        else:
                            edge = (destination_id, source_id)

                        if int(operation) == 0:
                            if edge in active_edges:
                                repeated_insertions += 1
                            else:
                                active_edges.add(edge)
                        elif int(operation) == 1:
                            if edge not in active_edges:
                                deletions_of_absent_edges += 1
                            else:
                                active_edges.remove(edge)

        stats["sha256"] = hasher.hexdigest()
        stats["timestamp_order_violations"] = timestamp_order_violations
        stats["invalid_timestamp_values"] = invalid_timestamp_values
        stats["out_of_range_vertex_ids"] = out_of_range_vertex_ids
        stats["negative_vertex_ids"] = negative_vertex_ids

        if not bool(signal_raw):
            stats["insertions"] = insertions
            stats["deletions"] = deletions
            stats["invalid_update_type_values"] = invalid_info_values
        if active_edges is not None:
            stats["repeated_insertions"] = repeated_insertions
            stats["deletions_of_absent_edges"] = deletions_of_absent_edges
            stats["active_edges_at_end"] = len(active_edges)

        if stats["rows_scanned"] != total_updates:
            errors.append(
                f"Scanned {stats['rows_scanned']} complete records, but the header "
                f"declares {total_updates}"
            )
        if negative_vertex_ids:
            errors.append(f"Found {negative_vertex_ids} records with negative vertex IDs")
        if out_of_range_vertex_ids:
            errors.append(
                f"Found {out_of_range_vertex_ids} records with vertex IDs outside "
                f"[0, {num_vertices})"
            )
        if timestamp_order_violations:
            errors.append(
                f"Found {timestamp_order_violations} timestamp ordering violations"
            )
        if invalid_timestamp_values:
            errors.append(
                f"Found {invalid_timestamp_values} non-finite timestamp values"
            )
        if invalid_info_values:
            errors.append(
                f"Found {invalid_info_values} update-type values other than 0 or 1"
            )
        if repeated_insertions:
            errors.append(
                f"Found {repeated_insertions} insertions of edges already present"
            )
        if deletions_of_absent_edges:
            errors.append(
                f"Found {deletions_of_absent_edges} deletions of absent edges"
            )

        max_vertex_id = stats["max_vertex_id"]
        if (
            max_vertex_id is not None
            and out_of_range_vertex_ids == 0
            and max_vertex_id + 1 < num_vertices
        ):
            warnings.append(
                "The largest observed vertex ID is smaller than num_vertices - 1; "
                "the ID range may not be contiguous"
            )
        if timestamp_code == 0:
            warnings.append("The dataset has no timestamp field")
        if stats["self_loops"]:
            warnings.append(
                f"The dataset contains {stats['self_loops']} self-loop records; "
                "confirm that the contribution documents their treatment"
            )

    report["valid"] = not errors
    return report


def _print_report(report: dict[str, Any]) -> None:
    status = "VALID" if report["valid"] else "INVALID"
    print(f"{status}: {report['path']}")
    header = report.get("header")
    if header:
        print(
            "  "
            f"vertices={header['num_vertices']}, "
            f"unique_edges={header['num_unique_edges']}, "
            f"updates={header['total_updates']}, "
            f"type={'signal' if header['signal'] else 'update'}, "
            f"directed={header['directed']}"
        )
    stats = report["statistics"]
    if stats.get("sha256"):
        print(f"  sha256={stats['sha256']}")
    for warning in report["warnings"]:
        print(f"  warning: {warning}")
    for error in report["errors"]:
        print(f"  error: {error}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Validate a GraphTide binary graph file"
    )
    parser.add_argument("path", help="Path to a GraphTide .bin file")
    parser.add_argument(
        "--check-update-sequence",
        action="store_true",
        help="Replay an update graph; memory use is proportional to active edges",
    )
    parser.add_argument(
        "--json",
        metavar="PATH",
        help="Also write the complete validation report as JSON",
    )
    parser.add_argument(
        "--chunk-rows",
        type=int,
        default=1_000_000,
        help="Number of records per streaming chunk (default: 1000000)",
    )
    args = parser.parse_args(argv)

    report = validate(
        args.path,
        check_update_sequence=args.check_update_sequence,
        chunk_rows=args.chunk_rows,
    )
    _print_report(report)
    if args.json:
        output = Path(args.json)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, indent=2) + "\n")
    return 0 if report["valid"] else 1


if __name__ == "__main__":
    raise SystemExit(main())

