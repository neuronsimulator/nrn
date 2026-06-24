#!/usr/bin/env python3
"""Compare test_par_gj voltages: CPU vs native GPU (gap junctions)."""

import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

_SCRIPT = Path(__file__).resolve().parent
_RECORD = _SCRIPT / "record_par_gj_voltages.py"


def run(native: bool, out_path: Path):
    subprocess.run(
        [sys.executable, str(_RECORD), str(int(native)), str(out_path)],
        cwd=_SCRIPT,
        check=True,
    )


def main():
    with tempfile.TemporaryDirectory() as tmp:
        cpu_path = Path(tmp) / "cpu.npy"
        gpu_path = Path(tmp) / "gpu.npy"
        run(False, cpu_path)
        run(True, gpu_path)
        v_cpu = np.load(cpu_path)
        v_gpu = np.load(gpu_path)
        max_diff = float(np.max(np.abs(v_cpu - v_gpu)))
        print("par_gj native_gpu max voltage diff:", max_diff)
        if max_diff > 1e-6:
            raise SystemExit(1)


if __name__ == "__main__":
    main()
