#!/usr/bin/env bash
# Product harness: Traub 1/10 native GPU spike multiset vs checked-in reference.
#
# Model stays outside the NEURON tree (ModelDB 82894). Ship tools + refs here.
#
# Usage (from any cwd; install on PATH via nrnenv):
#   source ~/neuron/bin/nrnenv nrngpu build-gpu
#   export NRN_GPU_BACKEND_TEST=native NRN_GPU_PERMUTE=2
#   bash test/external/traub/run_traub_native.sh            # no-gap → 4474
#   bash test/external/traub/run_traub_native.sh --gap       # gap → 7873
#   bash test/external/traub/run_traub_native.sh --rebuild   # force nrnivmodl
#
# Env:
#   NRN_TRAUB_MODEL     model root (default: $HOME/models/82894)
#   NRN_TRAUB_MECH_DIR  ACC special workdir (default: /tmp/traub-nrngpu-acc)
#   NRN_TRAUB_TSTOP     tstop ms (default: 100)
#   NRN_TRAUB_NTHREAD   nthread (default: 1)
#   NRN_GPU_PERMUTE     cell permute (default: 2)
#   NRN_TRAUB_SKIP_IF_NO_MODEL=1  exit 77 (ctest skip) if model missing
#
# Exit codes: 0 ok, 1 fail, 77 skip (no model / no GPU when requested).
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
# CTest copies this script into the build tree; refs stay in the source tree.
# Prefer NRN_TRAUB_REF_DIR (set by CMake); else sibling reference/ (source checkout).
REF_DIR=${NRN_TRAUB_REF_DIR:-${SCRIPT_DIR}/reference}

USE_GAP=0
REBUILD=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --gap) USE_GAP=1; shift ;;
    --no-gap) USE_GAP=0; shift ;;
    --rebuild) REBUILD=1; shift ;;
    -h|--help)
      sed -n '2,25p' "$0"
      exit 0
      ;;
    *)
      echo "unknown arg: $1" >&2
      exit 1
      ;;
  esac
done

MODEL=${NRN_TRAUB_MODEL:-${HOME}/models/82894}
MECH_DIR=${NRN_TRAUB_MECH_DIR:-/tmp/traub-nrngpu-acc}
TSTOP=${NRN_TRAUB_TSTOP:-100}
NTHREAD=${NRN_TRAUB_NTHREAD:-1}
PERMUTE=${NRN_GPU_PERMUTE:-2}
export NRN_GPU_BACKEND_TEST=${NRN_GPU_BACKEND_TEST:-native}
export NRN_GPU_PERMUTE=${PERMUTE}
export OMP_NUM_THREADS=${OMP_NUM_THREADS:-1}

if [[ ! -d "${MODEL}" || ! -f "${MODEL}/init.hoc" ]]; then
  msg="Traub model not found at ${MODEL} (set NRN_TRAUB_MODEL). Expected ModelDB 82894 layout."
  if [[ "${NRN_TRAUB_SKIP_IF_NO_MODEL:-0}" == "1" ]]; then
    echo "SKIP: ${msg}" >&2
    exit 77
  fi
  echo "ERROR: ${msg}" >&2
  exit 1
fi

if [[ ! -d "${MODEL}/mod" ]]; then
  echo "ERROR: ${MODEL}/mod missing" >&2
  exit 1
fi

if [[ "${USE_GAP}" -eq 1 ]]; then
  REF="${REF_DIR}/spikes_gap_100ms.srt"
  EXPECT=7873
else
  REF="${REF_DIR}/spikes_nogap_100ms.srt"
  EXPECT=4474
fi

if [[ ! -f "${REF}" ]]; then
  echo "ERROR: reference missing: ${REF}" >&2
  exit 1
fi

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "ERROR: '$1' not on PATH (source nrnenv / ninja install)" >&2
    exit 1
  }
}
need_cmd nrnivmodl
need_cmd nmodl
need_cmd sortspike

# --- build ACC special when missing, forced, or older than libnrniv ---
mkdir -p "${MECH_DIR}"
SPECIAL="${MECH_DIR}/x86_64/special"
LIBNRNIV=""
for cand in \
  "${NRNHOME:-}/lib/libnrniv.so" \
  "${NEURONHOME:-}/../../lib/libnrniv.so" \
  "$(dirname "$(command -v nrnivmodl)")/../lib/libnrniv.so"; do
  if [[ -f "${cand}" ]]; then
    LIBNRNIV=$(readlink -f "${cand}" 2>/dev/null || echo "${cand}")
    break
  fi
done

should_build=0
if [[ "${REBUILD}" -eq 1 ]]; then
  should_build=1
elif [[ ! -x "${SPECIAL}" ]]; then
  should_build=1
elif [[ -n "${LIBNRNIV}" && "${LIBNRNIV}" -nt "${SPECIAL}" ]]; then
  echo "Info: libnrniv newer than special → rebuild ACC Traub mechs"
  should_build=1
fi

if [[ "${should_build}" -eq 1 ]]; then
  echo "Building Traub ACC special in ${MECH_DIR} ..."
  # Fresh links so removed/renamed mods do not linger.
  find "${MECH_DIR}" -maxdepth 1 -type l -name '*.mod' -delete 2>/dev/null || true
  # shellcheck disable=SC2086
  ln -sfn ${MODEL}/mod/*.mod "${MECH_DIR}/"
  (
    cd "${MECH_DIR}"
    # Drop stale objects when libnrniv changes (NVHPC fat-object paths).
    rm -rf x86_64
    nrnivmodl -nmodl "$(command -v nmodl)" \
      -nmodlflags "passes --inline host --c acc --oacc" .
  )
  if [[ ! -x "${SPECIAL}" ]]; then
    echo "ERROR: nrnivmodl did not produce ${SPECIAL}" >&2
    exit 1
  fi
else
  echo "Reusing ACC special: ${SPECIAL}"
fi

WORK=$(mktemp -d "${TMPDIR:-/tmp}/traub-native.XXXXXX")
cleanup() { rm -rf "${WORK}"; }
trap cleanup EXIT

echo "Running Traub native GPU: use_gap=${USE_GAP} tstop=${TSTOP} nthread=${NTHREAD}"
# Run from model dir so relative hoc/mod paths resolve; spikes land as out1.dat
(
  cd "${MODEL}"
  rm -f out1.dat
  "${SPECIAL}" \
    -c "one_tenth_ncell=1" \
    -c "use_gap=${USE_GAP}" \
    -c "nthread=${NTHREAD}" \
    -c "enable_gpu=1" \
    -c "coreneuron=0" \
    -c "mytstop=${TSTOP}" \
    -c "benchmark_quiet=1" \
    init.hoc
)

if [[ ! -f "${MODEL}/out1.dat" ]]; then
  echo "ERROR: missing ${MODEL}/out1.dat after run" >&2
  exit 1
fi

cp -f "${MODEL}/out1.dat" "${WORK}/out1.dat"
sortspike "${WORK}/out1.dat" "${WORK}/spikes.srt"
GOT=$(grep -c . "${WORK}/spikes.srt" || true)
echo "spikes=${GOT} (expect ${EXPECT})"

if [[ "${GOT}" -ne "${EXPECT}" ]]; then
  echo "ERROR: spike count ${GOT} != ${EXPECT}" >&2
  exit 1
fi

if ! cmp -s "${WORK}/spikes.srt" "${REF}"; then
  echo "ERROR: sorted spike multiset differs from ${REF}" >&2
  # Show a short diff head for triage
  diff -u "${REF}" "${WORK}/spikes.srt" | head -40 || true
  exit 1
fi

echo "OK: Traub native use_gap=${USE_GAP} exact match (${EXPECT} spikes) vs $(basename "${REF}")"
