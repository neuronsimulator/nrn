#!/usr/bin/env bash
# CPU vs native-GPU prcellstate for a dentate GC (default gid 500006).
#
# Usage (from reduced_dentate_native workdir with ACC special on PATH):
#   bash .../prcellstate_gc_native.sh [GID] [TSTOP] [0|1]
#     0 = end-of-run dump only (default)
#     1 = also arm fixed-step phase dumps at TSTOP
#
# Progressive ladder:
#   1) End-of-run at early tstop (e.g. 5.0) — expect near-match before GC spike
#   2) Bisect / step toward first GC spike (5.525 for gids 500006/500009)
#   3) phases=1 at first bad tstop → post_setup / post_solve / pre_nonvint / post_nonvint
#   4) rdcellstate field diffs (na8st, CadepK, V, matrix, …)
#
# Compare:
#   python3 ~/models/82894/rdcellstate.py --ignore-unused \
#     ${GID}_cpu-t${TSTOP}.nrndat ${GID}_gpu-t${TSTOP}.nrndat
set -euo pipefail

gid="${1:-500006}"
tstop="${2:-5.5}"
phases="${3:-0}"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
py="${script_dir}/run_dentate_prcellstate.py"
rdcellstate="${RDCELLSTATE:-$HOME/models/82894/rdcellstate.py}"

if ! command -v special >/dev/null; then
  echo "error: special not on PATH (use dentate ACC special)" >&2
  exit 1
fi

ckpt_env=()
case "$phases" in
0) ;;
1) ckpt_env=(NRN_PRCELLSTATE_CHECKPOINT_T="$tstop") ;;
*)
  echo "usage: $0 [GID] [TSTOP] [0|1]" >&2
  exit 1
  ;;
esac

run_one() {
  local gpu="$1" tag="$2"
  env NRN_GPU="$gpu" \
      NRN_TEST_TSTOP="$tstop" \
      NRN_TEST_MAX_CELLS=100 \
      NRN_PRCELLSTATE_GID="$gid" \
      NRN_PRCELLSTATE_TAG="$tag" \
      NRN_GPU_BACKEND_TEST=native \
      NRN_GPU_PERMUTE=2 \
      OMP_NUM_THREADS=1 \
      HOC_LIBRARY_PATH=templates \
      "${ckpt_env[@]+"${ckpt_env[@]}"}" \
      mpiexec -n 1 special -notatty -python "$py"
}

echo "=== dentate prcellstate gid=$gid tstop=$tstop phases=$phases ==="
run_one 0 cpu
run_one 1 gpu

# HOC %g may print 5 instead of 5.0 — resolve actual dump names.
cpu="$(ls -1 ${gid}_cpu-t*.nrndat 2>/dev/null | sort | tail -1 || true)"
gpu="$(ls -1 ${gid}_gpu-t*.nrndat 2>/dev/null | sort | tail -1 || true)"
if [[ -z ${cpu:-} || -z ${gpu:-} ]]; then
  echo "error: missing dumps for gid=$gid (cpu='$cpu' gpu='$gpu')" >&2
  ls -la ./*"${gid}"* 2>/dev/null || true
  exit 1
fi

echo "=== rdcellstate $cpu vs $gpu ==="
python3 "$rdcellstate" --ignore-unused "$cpu" "$gpu" || true
echo "exit note: rdcellstate exit 1 on any field noise is normal; inspect Top diffs"
