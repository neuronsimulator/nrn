#!/usr/bin/env bash
# Reproduce ringtest CPU vs native-GPU spike parity (default and -rparm).
set -euo pipefail

BUILD=`dirname $N`
RT="${BUILD}/test/external_ringtest"
WORK_DIR="${RT}/neuron_gpu_native_mpi"
NMODLHOME="${BUILD}/install"

export NMODLHOME
export PATH="${NMODLHOME}/bin:${PATH}"
export LD_LIBRARY_PATH="${BUILD}/lib:${NMODLHOME}/lib:${LD_LIBRARY_PATH}"
export PYTHONPATH="${NMODLHOME}/lib/python:${PYTHONPATH}"
export OMP_NUM_THREADS=1
export NRN_GPU_BACKEND_TEST=native
export NRN_GPU_PERMUTE=2

build_special() {
  local dir="$1"
  echo "==> nrnivmodl -coreneuron in ${dir}"
  (
    cd "${dir}"
    rm -f x86_64/libnrnmech.so x86_64/special x86_64/special-core
    nrnivmodl -coreneuron .
  )
}

run_ringtest() {
  local dir="$1"
  local label="$2"
  local nhost="$3"
  shift 3
  echo ""
  echo "==> ${label}"
  (
    cd "${dir}"
    rm -f spk${nhost}.std
    export PATH="${dir}/x86_64:${NMODLHOME}/bin:${PATH}"
    export LD_LIBRARY_PATH="${BUILD}/lib:${dir}/x86_64:${NMODLHOME}/lib:${LD_LIBRARY_PATH}"
    echo mpiexec -n ${nhost} --oversubscribe special -mpi -python ringtest.py "$@"
    mpiexec -n ${nhost} --oversubscribe special -mpi -python ringtest.py "$@"
    tmp=$(mktemp)
    sortspike "spk${nhost}.std" "$tmp"
    mv "$tmp" "spk${nhost}.std"
  )
  local n
  n=$(wc -l < "${dir}/spk${nhost}.std")
  echo "    wrote ${n} spikes -> ${dir}/spk${nhost}.std"
}

compare_spikes() {
  local cpu_file="$1"
  local gpu_file="$2"
  local label="$3"
  echo "==> diff ${label}"
  diff -u "${cpu_file}" "${gpu_file}"
  echo "    OK: CPU and GPU spikes identical"
}

build_special "${WORK_DIR}"

nhost=1
common="-rparm -tstop 100 -nring 128"

run_ringtest "${WORK_DIR}" "CPU" ${nhost} ${common}
cp "${WORK_DIR}/spk${nhost}.std" /tmp/ringtest_cpu_rparm.std

run_ringtest "${WORK_DIR}" "GPU CN" ${nhost} -coreneuron -gpu ${common}
compare_spikes /tmp/ringtest_cpu_rparm.std "${WORK_DIR}/spk${nhost}.std" "GPU-CN"

run_ringtest "${WORK_DIR}" "GPU native" ${nhost} -gpu-native ${common}
compare_spikes /tmp/ringtest_cpu_rparm.std "${WORK_DIR}/spk${nhost}.std" "GPU-native"

# Device nonvint is mandatory under native (no separate NONVINT env run).

echo "==> All ringtest comparisons passed."
