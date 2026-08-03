"""Enable NEURON native GPU then load reduced_dentate run.hoc (MPI product).

Launch (from the nrn_add_test workdir that has ACC special + datasets):

  # Preferred product path (ensures CUDA MPS for multi-rank on one GPU):
  bash /path/to/run_dentate_native_mpi.sh
  # or CTest: reduced_dentate_native::neuron_gpu_native

  # Manual (start MPS first if ranks/GPU > 1):
  nvidia-cuda-mps-control -d   # once per node
  mpiexec -n 4 special -notatty -python /path/to/run_dentate_native.py

Uses h.nrnmpi_init() (not special -mpi) for OpenACC multi-process safety.
Env: NRN_TEST_TSTOP (default 10), NRN_TEST_MAX_CELLS (default 100).

Without CUDA MPS, 4 ranks on 1 GPU are often ~10× slower (~37 s vs ~3 s psolve).
See test/external/ensure_cuda_mps.sh and docs/dev/native-gpu-build.rst.
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
