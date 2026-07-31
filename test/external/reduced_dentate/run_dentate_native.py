"""Enable NEURON native GPU then load reduced_dentate run.hoc (MPI product).

Launch (from the nrn_add_test workdir that has ACC special + datasets):

  mpiexec -n 4 special -notatty -c mytstop=10 -c max_cells_per_type=100 \\
      -python /path/to/run_dentate_native.py

Uses h.nrnmpi_init() (not special -mpi) for OpenACC multi-process safety.
Pre-declare mytstop/max_cells via special -c before -python when possible;
this script also sets defaults if missing.
"""
from __future__ import annotations

import os
import sys

from neuron import gpu, h


def main() -> None:
    # run.hoc does nrnpython("from commonutils import mkdir_p") — workdir must
    # be on sys.path (ctest does not put CWD on PYTHONPATH by default).
    cwd = os.getcwd()
    if cwd not in sys.path:
        sys.path.insert(0, cwd)
    h.nrnmpi_init()
    # Product defaults (special -c under -python is unreliable; env optional override).
    mytstop = float(os.environ.get("NRN_TEST_TSTOP", "10"))
    max_cells = int(os.environ.get("NRN_TEST_MAX_CELLS", "100"))
    h(f"mytstop = {mytstop}")
    h(f"max_cells_per_type = {max_cells}")
    h("coreneuron = 0")
    h("gpu = 0")
    gpu.backend = "native"
    gpu.permute = int(os.environ.get("NRN_GPU_PERMUTE", "2"))
    gpu.enable = True
    h.load_file("run.hoc")


if __name__ == "__main__":
    main()
