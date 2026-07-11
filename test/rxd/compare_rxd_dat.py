#!/usr/bin/env python3
import sys
import os
import argparse
import numpy as np
import matplotlib.pyplot as plt


def load_dat(filepath):
    """Load RxD .dat binary file (typically float64, possibly with metadata)"""
    try:
        data = np.fromfile(filepath, dtype=np.float64)
        print(f"Loaded {filepath}: {len(data)} floats")
        return data
    except Exception as e:
        print(f"Error loading {filepath}: {e}")
        return None


def compare_two_files(file1, file2, title="RxD Data Comparison", plot=True, tol=1e-10):
    d1 = load_dat(file1)
    d2 = load_dat(file2)
    if d1 is None or d2 is None:
        return

    min_len = min(len(d1), len(d2))
    diff = d1[:min_len] - d2[:min_len]
    max_abs_diff = np.max(np.abs(diff))
    rms_diff = np.sqrt(np.mean(diff**2))
    mean_abs_diff = np.mean(np.abs(diff))

    print(
        f"\n=== Comparison: {os.path.basename(file1)} vs {os.path.basename(file2)} ==="
    )
    print(f"Lengths: {len(d1)} vs {len(d2)}")
    print(f"Max abs diff : {max_abs_diff:.2e}")
    print(f"RMS diff     : {rms_diff:.2e}")
    print(f"Mean abs diff: {mean_abs_diff:.2e}")
    print(f"Tolerance    : {tol}")

    if max_abs_diff > tol:
        print("X Differences exceed tolerance!")
    else:
        print("Within tolerance.")

    if plot:
        plt.figure(figsize=(14, 10))

        # Raw traces (downsampled)
        step = max(1, min_len // 5000)  # adaptive downsampling
        plt.subplot(3, 1, 1)
        plt.plot(d1[::step], label=os.path.basename(file1), alpha=0.85)
        plt.plot(d2[::step], label=os.path.basename(file2), alpha=0.85)
        plt.title(f"{title} - Traces (downsampled)")
        plt.legend()
        plt.grid(True)

        # Difference
        plt.subplot(3, 1, 2)
        plt.plot(diff[::step])
        plt.title("Pointwise Difference")
        plt.grid(True)

        # Absolute diff (log)
        plt.subplot(3, 1, 3)
        plt.plot(np.abs(diff[::step]))
        plt.axhline(tol, color="r", linestyle="--", label=f"Tol = {tol}")
        plt.yscale("log")
        plt.title("Absolute Difference (log scale)")
        plt.legend()
        plt.grid(True)

        plt.tight_layout()
        plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compare two RxD .dat binary files")
    parser.add_argument("file1", help="First .dat file")
    parser.add_argument("file2", help="Second .dat file")
    parser.add_argument("--no-plot", action="store_true")
    parser.add_argument("--tol", type=float, default=1e-10)
    args = parser.parse_args()

    compare_two_files(args.file1, args.file2, plot=not args.no_plot, tol=args.tol)
