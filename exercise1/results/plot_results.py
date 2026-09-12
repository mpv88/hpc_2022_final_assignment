#!/usr/bin/env python3

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


DEFAULT_CSV_DIR = Path(__file__).resolve().parent          # results/
DEFAULT_FIG_DIR = Path(__file__).resolve().parent.parent / "figs"  # exercise1/figs/

COLUMNS = [
    "evolution", "grid_width", "grid_height", "steps",
    "mpi_tasks", "omp_threads", "repetition",
    "read_time", "evolution_time", "write_time", "total_time",
]

def parse_args():
    p = argparse.ArgumentParser(description="Plot Game of Life HPC benchmark results")
    p.add_argument(
        "csv_files", nargs="*", type=Path,
        help="CSV files (default: all *.csv in results/)"
    )
    p.add_argument(
        "--out", type=Path, default=DEFAULT_FIG_DIR,
        help=f"output directory for plots (default: {DEFAULT_FIG_DIR})"
    )
    p.add_argument(
        "--x", choices=["mpi_tasks", "omp_threads"], default=None,
        help="x-axis variable (default: infer from data)"
    )
    return p.parse_args()

def load_data(csv_files):
    frames = []
    for path in csv_files:
        df = pd.read_csv(path)
        if not set(COLUMNS).issubset(df.columns):
            missing = set(COLUMNS) - set(df.columns)
            raise ValueError(f"{path}: missing columns {missing}")
        frames.append(df)

    data = pd.concat(frames, ignore_index=True)
    num_cols = [
        "grid_width", "grid_height", "steps",
        "mpi_tasks", "omp_threads", "repetition",
        "read_time", "evolution_time", "write_time", "total_time",
    ]
    for c in num_cols:
        data[c] = pd.to_numeric(data[c], errors="raise")
    return data

def aggregate(data):
    group_cols = ["evolution", "grid_width", "grid_height", "steps", "mpi_tasks", "omp_threads"]
    agg = (
        data.groupby(group_cols, as_index=False)["evolution_time"]
        .agg(["mean", "std", "count"])
        .reset_index()
    )
    agg["std"] = agg["std"].fillna(0.0)
    return agg

def plot_band(ax, df, x, y, yerr, label):
    df = df.sort_values(x)
    ax.plot(df[x], df[y], marker="o", label=label)
    ax.fill_between(
        df[x],
        df[y] - df[yerr],
        df[y] + df[yerr],
        alpha=0.25, linewidth=0, edgecolor=None
    )

def make_plots(data, out_dir, x_col):
    agg = aggregate(data)
    out_dir.mkdir(parents=True, exist_ok=True)

    # --- speedup ---
    fig, ax = plt.subplots()
    for evol, df in agg.groupby("evolution"):
        df = df.sort_values(x_col).copy()
        base = df.iloc[0]["mean"]
        df["speedup"] = base / df["mean"]
        df["speedup_err"] = df["speedup"] * df["std"] / df["mean"]
        plot_band(ax, df, x_col, "speedup", "speedup_err", evol)

    xmin, xmax = agg[x_col].min(), agg[x_col].max()
    ax.plot([xmin, xmax], [1, xmax / xmin], "--", label="ideal")
    ax.set_title("Strong scaling speedup")
    ax.set_xlabel("OMP threads" if x_col == "omp_threads" else "MPI tasks")
    ax.set_ylabel("Speedup")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(out_dir / f"speedup_{x_col}.png", dpi=200, bbox_inches="tight")
    plt.close(fig)

    # --- time ---
    fig, ax = plt.subplots()
    for evol, df in agg.groupby("evolution"):
        df = df.rename(columns={"mean": "time", "std": "time_std"})
        plot_band(ax, df, x_col, "time", "time_std", evol)

    ax.set_title("Execution time")
    ax.set_xlabel("OMP threads" if x_col == "omp_threads" else "MPI tasks")
    ax.set_ylabel("Evolution time (s)")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(out_dir / f"time_{x_col}.png", dpi=200, bbox_inches="tight")
    plt.close(fig)

    # --- efficiency ---
    fig, ax = plt.subplots()
    for evol, df in agg.groupby("evolution"):
        df = df.sort_values(x_col).copy()
        base = df.iloc[0]["mean"]
        df["eff"] = base / (df["mean"] * df[x_col])
        df["eff_err"] = df["eff"] * df["std"] / df["mean"]
        plot_band(ax, df, x_col, "eff", "eff_err", evol)

    ax.axhline(1.0, linestyle="--", label="ideal")
    ax.set_title("Parallel efficiency")
    ax.set_xlabel("OMP threads" if x_col == "omp_threads" else "MPI tasks")
    ax.set_ylabel("Efficiency")
    ax.set_ylim(bottom=0)
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(out_dir / f"efficiency_{x_col}.png", dpi=200, bbox_inches="tight")
    plt.close(fig)

def main():
    args = parse_args()

    csv_files = args.csv_files
    if not csv_files:
        csv_files = sorted(DEFAULT_CSV_DIR.glob("*.csv"))
        if not csv_files:
            raise SystemExit(f"No CSV files found in {DEFAULT_CSV_DIR}")

    data = load_data(csv_files)

    # infer x-axis if not given
    x_col = args.x
    if x_col is None:
        if "omp_threads" in data.columns and data["omp_threads"].nunique() > 1:
            x_col = "omp_threads"
        elif "mpi_tasks" in data.columns and data["mpi_tasks"].nunique() > 1:
            x_col = "mpi_tasks"
        else:
            raise SystemExit("Cannot infer x-axis; use --x mpi_tasks or --x omp_threads")

    make_plots(data, args.out, x_col)
    print(f"Plots saved to: {args.out}")

if __name__ == "__main__":
    main()