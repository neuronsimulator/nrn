"""Enable NEURON native GPU then load a testcorenrn HOC script.

Used by P3 ctests (testcorenrn_*_native). Command shape:

  special -notatty -c arg_tstop=100 -python run_native_gpu.py testconc.hoc

Requires NRN_GPU_BACKEND_TEST=native (or relies on explicit gpu.enable here)
and an ACC-built special for Gate B/C mechs in the model.
"""
from __future__ import annotations

import os
import sys

from neuron import gpu, h


def main(argv: list[str]) -> None:
    hoc = argv[1] if len(argv) > 1 else "testconc.hoc"
    perm = int(os.environ.get("NRN_GPU_PERMUTE", "2"))
    gpu.backend = "native"
    gpu.permute = perm
    gpu.enable = True
    # HOC scripts load defvar/common and run to completion (often call quit()).
    h.load_file(hoc)


if __name__ == "__main__":
    main(sys.argv)
