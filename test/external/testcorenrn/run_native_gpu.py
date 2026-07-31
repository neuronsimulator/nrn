"""Enable NEURON native GPU then load a testcorenrn HOC script.

Used by P3 ctests (testcorenrn_*_native). Command shape:

  special -notatty -python run_native_gpu.py

Do **not** pass the .hoc as a trailing special argument — special treats
trailing *.hoc as host scripts and runs them before (or instead of) enabling
native GPU. Select the model via env instead.

Env:
  NRN_TEST_HOC      (required) e.g. testconc.hoc
  NRN_GPU_PERMUTE   (default 2)
  NRN_TEST_TSTOP    (default 100) — pre-declared as arg_tstop so defvar.hoc
                    keeps this value (special -c is unreliable under -python).

Requires ACC-built specials for Gate B/C mechs when the model has CURRENT/STATE
density/point processes (builtins alone may QUALIFY for NetStim-only models).
"""
from __future__ import annotations

import os
import sys

from neuron import gpu, h


def main(argv: list[str]) -> None:
    hoc = os.environ.get("NRN_TEST_HOC") or (argv[1] if len(argv) > 1 else "")
    if not hoc:
        sys.stderr.write(
            "run_native_gpu.py: set NRN_TEST_HOC=testFOO.hoc "
            "(do not pass the hoc as a special trailing arg)\n"
        )
        sys.exit(2)
    perm = int(os.environ.get("NRN_GPU_PERMUTE", "2"))
    tstop = float(os.environ.get("NRN_TEST_TSTOP", "100"))
    # Pre-declare before any load_file("defvar.hoc") so default_var keeps it.
    h(f"arg_tstop = {tstop}")
    gpu.backend = "native"
    gpu.permute = perm
    gpu.enable = True
    # HOC scripts load defvar/common and run to completion (often call quit()).
    h.load_file(hoc)


if __name__ == "__main__":
    main(sys.argv)
