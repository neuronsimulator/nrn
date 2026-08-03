#!/usr/bin/env bash
# Product multi-rank launcher for reduced_dentate native GPU.
#
# Ensures CUDA MPS (multi-rank-on-1-GPU policy), then mpiexec special -python
# with h.nrnmpi_init via run_dentate_native.py (not special -mpi).
#
# Env (optional):
#   NRN_TEST_TSTOP       default 10
#   NRN_TEST_MAX_CELLS   default 100
#   NRN_GPU_PERMUTE      default 2
#   NRN_DENTATE_RANKS    default 4
#   NRN_SKIP_CUDA_MPS=1  skip ensure_cuda_mps.sh
#
# CTest wires this from CMake; manual:
#   source ~/neuron/bin/nrnenv nrngpu build-gpu
#   cd build-gpu/test/reduced_dentate_native/neuron_gpu_native   # or workdir with special
#   bash /path/to/run_dentate_native_mpi.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# When installed/copied next to CMake workdir, ensure script lives in test/external/
ENSURE_MPS="${NRN_ENSURE_CUDA_MPS:-}"
if [[ -z "${ENSURE_MPS}" ]]; then
  if [[ -x "${SCRIPT_DIR}/../ensure_cuda_mps.sh" ]]; then
    ENSURE_MPS="${SCRIPT_DIR}/../ensure_cuda_mps.sh"
  elif [[ -x "${SCRIPT_DIR}/ensure_cuda_mps.sh" ]]; then
    ENSURE_MPS="${SCRIPT_DIR}/ensure_cuda_mps.sh"
  fi
fi

ranks="${NRN_DENTATE_RANKS:-4}"
run_py="${NRN_DENTATE_RUN_PY:-${SCRIPT_DIR}/run_dentate_native.py}"

export OMP_NUM_THREADS="${OMP_NUM_THREADS:-1}"
export NRN_GPU_BACKEND_TEST="${NRN_GPU_BACKEND_TEST:-native}"
export NRN_GPU_PERMUTE="${NRN_GPU_PERMUTE:-2}"
export NRN_TEST_TSTOP="${NRN_TEST_TSTOP:-10}"
export NRN_TEST_MAX_CELLS="${NRN_TEST_MAX_CELLS:-100}"

if [[ -n "${ENSURE_MPS}" && -x "${ENSURE_MPS}" ]]; then
  bash "${ENSURE_MPS}"
fi

# Prefer mpiexec from PATH (nrnenv / OpenMPI).
mpiexec_bin="${MPIEXEC:-mpiexec}"
exec "${mpiexec_bin}" -n "${ranks}" --oversubscribe \
  special -notatty -python "${run_py}"
