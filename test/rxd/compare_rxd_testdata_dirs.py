#!/usr/bin/env python3
import sys
import os
import argparse
import hashlib
import subprocess
from pathlib import Path


def get_all_dat_files(dir1: Path, dir2: Path):
    """Return union of .dat files from both directories (recursive)."""
    files1 = {p.relative_to(dir1): p for p in dir1.rglob("*.dat")}
    files2 = {p.relative_to(dir2): p for p in dir2.rglob("*.dat")}

    all_rel = sorted(set(files1.keys()) | set(files2.keys()))
    return all_rel, files1, files2


def files_differ(file1: Path, file2: Path) -> bool:
    """Quick check if two files differ (size + hash)."""
    if not file1.exists() or not file2.exists():
        return True
    if file1.stat().st_size != file2.stat().st_size:
        return True
    with open(file1, "rb") as f1, open(file2, "rb") as f2:
        return hashlib.md5(f1.read()).digest() != hashlib.md5(f2.read()).digest()


def main():
    parser = argparse.ArgumentParser(
        description="Compare two RxD testdata directories pairwise"
    )
    parser.add_argument(
        "dir1", type=Path, help="First testdata directory (e.g. master)"
    )
    parser.add_argument(
        "dir2", type=Path, help="Second testdata directory (e.g. slds or new)"
    )
    parser.add_argument(
        "--compare-script",
        type=Path,
        default=Path("compare_rxd_dat.py"),
        help="Path to compare_rxd_dat.py",
    )
    parser.add_argument("--tol", type=float, default=1e-10)
    parser.add_argument("--no-plot", action="store_true")
    args = parser.parse_args()

    if not args.dir1.exists() or not args.dir2.exists():
        print("Error: One or both directories do not exist.")
        sys.exit(1)

    compare_script = args.compare_script.resolve()
    if not compare_script.exists():
        print(f"Error: {compare_script} not found. Please create it first.")
        compare_script = Path(f"{os.path.dirname(__file__)}/compare_rxd_dat.py")
        print(f" Trying: {compare_script}")
        if not compare_script.exists():
            sys.exit(1)

    rel_files, files1, files2 = get_all_dat_files(args.dir1, args.dir2)

    print(f"Found {len(rel_files)} unique .dat files across both directories.\n")

    # Collect commands for differing pairs
    commands = []
    missing = []
    for rel in rel_files:
        f1 = files1.get(rel)
        f2 = files2.get(rel)

        if f1 is None:
            missing.append(f"Missing in dir1: {rel}")
            continue
        if f2 is None:
            missing.append(f"Missing in dir2: {rel}")
            continue

        if files_differ(f1, f2):
            cmd = [str(compare_script), str(f1), str(f2), "--tol", str(args.tol)]
            if args.no_plot:
                cmd.append("--no-plot")
            commands.append((rel, cmd))

    # Report missing files
    for m in missing:
        print(f"X {m}")
    if missing:
        print()

    if not commands:
        print("All corresponding files are identical (or no overlapping files).")
        return

    print(f"Will compare {len(commands)} differing pairs.\n")
    print("Commands that will be run:")
    for _, cmd in commands:
        print("  " + " ".join(cmd))
    print("\n" + "=" * 80)

    # Interactive loop
    for i, (rel, cmd) in enumerate(commands, 1):
        print(f"\n[{i}/{len(commands)}] Comparing: {rel}")
        print("Running:", " ".join(cmd))

        print(
            "\nClose the matplot window to continue to next pair (or Ctrl+C to stop)..."
        )
        try:
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError as e:
            print(f"Comparison script failed with exit code {e.returncode}")


if __name__ == "__main__":
    main()
