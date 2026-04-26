"""Run the C++ analysis binary on a DGDC binary graph file."""

from __future__ import annotations

import os
import shutil
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

import pandas as pd

# ── Locate the analysis binary ───────────────────────────────────────────────

_REPO_ROOT = Path(__file__).resolve().parent.parent
_DEFAULT_BUILD_DIR = _REPO_ROOT / "analyze" / "build"


def _find_binary(name: str = "run") -> Path:
    """Search for the compiled analysis binary."""
    # Check common build locations
    candidates = [
        _DEFAULT_BUILD_DIR / name,
        _DEFAULT_BUILD_DIR / "Release" / name,
        _DEFAULT_BUILD_DIR / "RelWithDebInfo" / name,
        _REPO_ROOT / "analyze" / name,
    ]
    for p in candidates:
        if p.is_file() and os.access(p, os.X_OK):
            return p

    # Fall back to PATH
    found = shutil.which(name)
    if found:
        return Path(found)

    raise FileNotFoundError(
        f"Cannot find the '{name}' binary. "
        f"Build it first: cd analyze && mkdir -p build && cd build "
        f"&& cmake .. && make -j"
    )


# ── Result container ─────────────────────────────────────────────────────────


@dataclass
class AnalysisResult:
    """Container for analysis output."""

    dynamic: Optional[pd.DataFrame] = None
    static: Optional[pd.DataFrame] = None
    output_dir: Path = field(default_factory=lambda: Path("."))


# ── Public API ───────────────────────────────────────────────────────────────


def analyze(
    input: str | Path,
    *,
    output_dir: str | Path | None = None,
    print_freq: int = 1000,
    static_algorithm_count: int = 0,
    src: int = 1,
    max_updates: int = -1,
    window_size: int = 0,
    edges_shuffle: bool = False,
    binary: str | Path | None = None,
) -> AnalysisResult:
    """Run graph_statistics on a DGDC binary file and return results as DataFrames.

    Parameters
    ----------
    input : path
        Path to a DGDC .bin file.
    output_dir : path, optional
        Directory for CSV outputs. Defaults to a temp dir next to the input.
    print_freq : int
        How often (in edge updates) to record dynamic statistics.
    static_algorithm_count : int
        How many times to run static algorithms (0 = skip).
    src : int
        Source vertex for BFS.
    max_updates : int
        Max edge updates to process (-1 = all).
    window_size : int
        For signal graphs: window size for converting signals to updates.
    edges_shuffle : bool
        Shuffle edge order.
    binary : path, optional
        Explicit path to the analysis binary.

    Returns
    -------
    AnalysisResult
        .dynamic : DataFrame from statistics.csv (per-timestep stats)
        .static  : DataFrame from static.csv (periodic snapshot stats), or None
        .output_dir : Path to the output directory
    """
    input = Path(input).resolve()
    if not input.is_file():
        raise FileNotFoundError(f"Input file not found: {input}")

    if output_dir is None:
        output_dir = input.parent / f"{input.stem}_analysis"
    output_dir = Path(output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    bin_path = Path(binary) if binary else _find_binary()

    cmd = [
        str(bin_path),
        f"--input={input}",
        f"--output={output_dir}",
        f"--print_freq={print_freq}",
        f"--static_algorithm_count={static_algorithm_count}",
        f"--src={src}",
        f"--max_updates={max_updates}",
        f"--window_size={window_size}",
    ]
    if edges_shuffle:
        cmd.append("--edges_shuffle=true")

    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"Analysis binary failed (exit {proc.returncode}):\n"
            f"stdout: {proc.stdout}\n"
            f"stderr: {proc.stderr}"
        )

    result = AnalysisResult(output_dir=output_dir)

    stats_csv = output_dir / "statistics.csv"
    if stats_csv.is_file():
        df = pd.read_csv(stats_csv, skipinitialspace=True)
        result.dynamic = df.loc[:, ~df.columns.str.startswith("Unnamed")]

    static_csv = output_dir / "static.csv"
    if static_csv.is_file():
        df = pd.read_csv(static_csv, skipinitialspace=True)
        result.static = df.loc[:, ~df.columns.str.startswith("Unnamed")]

    return result
