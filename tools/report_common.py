"""Shared helpers for the laboratory report notebooks.

Import from a notebook with::

    import sys
    sys.path.insert(0, str(ROOT / "tools"))
    import report_common as rc
"""

from __future__ import annotations

import subprocess
from pathlib import Path

import matplotlib as mpl
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data"
SEED = 12345
RUNS = 200

ALL_LABELS = {
    "random": "Random",
    "nn_end": "NN (append at end)",
    "nn_all": "NN (any position)",
    "greedy_cycle": "Greedy cycle",
    "regret_path": "2-regret (open path)",
    "regret_cycle": "2-regret (greedy cycle)",
    "weighted_path": "Weighted sum (open path)",
    "weighted_cycle": "Weighted sum (greedy cycle)",
    "ls_steepest_nodes_random": "Steepest, node swap, random start",
    "ls_steepest_edges_random": "Steepest, edge swap, random start",
    "ls_steepest_nodes_greedy": "Steepest, node swap, greedy start",
    "ls_steepest_edges_greedy": "Steepest, edge swap, greedy start",
    "ls_greedy_nodes_random": "Greedy, node swap, random start",
    "ls_greedy_edges_random": "Greedy, edge swap, random start",
    "ls_greedy_nodes_greedy": "Greedy, node swap, greedy start",
    "ls_greedy_edges_greedy": "Greedy, edge swap, greedy start",
}

ALL_SHORT = {
    "random": "Random",
    "nn_end": "NN append",
    "nn_all": "NN any pos.",
    "greedy_cycle": "Greedy cycle",
    "regret_path": "Regret path",
    "regret_cycle": "Regret cycle",
    "weighted_path": "Wsum path",
    "weighted_cycle": "Wsum cycle",
    "ls_steepest_nodes_random": "S/node/rand",
    "ls_steepest_edges_random": "S/edge/rand",
    "ls_steepest_nodes_greedy": "S/node/greedy",
    "ls_steepest_edges_greedy": "S/edge/greedy",
    "ls_greedy_nodes_random": "G/node/rand",
    "ls_greedy_edges_random": "G/edge/rand",
    "ls_greedy_nodes_greedy": "G/node/greedy",
    "ls_greedy_edges_greedy": "G/edge/greedy",
}

ALL_COLORS = {
    "random": "#9aa0a6",
    "nn_end": "#4c72b0",
    "nn_all": "#dd8452",
    "greedy_cycle": "#55a868",
    "regret_path": "#c44e52",
    "regret_cycle": "#8172b3",
    "weighted_path": "#937860",
    "weighted_cycle": "#da8bc3",
    "ls_steepest_nodes_random": "#1f77b4",
    "ls_steepest_edges_random": "#17becf",
    "ls_steepest_nodes_greedy": "#2ca02c",
    "ls_steepest_edges_greedy": "#98df8a",
    "ls_greedy_nodes_random": "#d62728",
    "ls_greedy_edges_random": "#ff9896",
    "ls_greedy_nodes_greedy": "#9467bd",
    "ls_greedy_edges_greedy": "#c5b0d5",
}


def build_engine() -> None:
    result = subprocess.run(["make", "-C", str(ROOT)], capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stdout + result.stderr)
        raise RuntimeError("build failed")


def run_solver(lab: str, out_dir: Path, extra_args: list[str] | None = None) -> str:
    out_dir.mkdir(parents=True, exist_ok=True)
    command = [
        str(ROOT / "tsp_solver"),
        "--seed", str(SEED),
        "--runs", str(RUNS),
        "--out", str(out_dir),
        "--data", str(DATA),
        "--lab", lab,
    ]
    if extra_args:
        command.extend(extra_args)
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    print(result.stdout.strip())
    if result.returncode != 0:
        print(result.stderr)
        raise RuntimeError("solver failed")
    return result.stdout


def load_results(out_dir: Path) -> tuple[pd.DataFrame, pd.DataFrame, pd.DataFrame]:
    summary = pd.read_csv(out_dir / "summary.csv")
    runs = pd.read_csv(out_dir / "runs.csv")
    tours = pd.read_csv(out_dir / "best_tours.csv")
    return summary, runs, tours


def load_coords(instances: list[str]) -> dict[str, pd.DataFrame]:
    return {
        name: pd.read_csv(DATA / f"{name}.csv", sep=";", header=None,
                          names=["x", "y", "cost"])
        for name in instances
    }


def configure_matplotlib() -> None:
    mpl.rcParams.update({
        "figure.dpi": 150,
        "savefig.dpi": 200,
        "font.size": 8,
        "axes.titlesize": 8.5,
        "axes.labelsize": 8,
        "xtick.labelsize": 7.5,
        "ytick.labelsize": 7.5,
    })


def nf(value: float) -> str:
    return f"{int(value):,}"


def f1(value: float) -> str:
    return f"{value:,.1f}"
