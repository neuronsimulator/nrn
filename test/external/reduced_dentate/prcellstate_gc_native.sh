#!/usr/bin/env bash
# CPU vs native-GPU prcellstate for a dentate GC (default gid 500006).
#
# Usage (from reduced_dentate_native workdir with ACC special on PATH):
#   bash .../prcellstate_gc_native.sh [GID] [TSTOP] [0|1]
#     0 = end-of-run dump only (default)
#     1 = also arm fixed-step phase dumps at TSTOP
#
# Progressive ladder:
#   1) End-of-run at early tstop (e.g. 0.05 or 5.0) — expect dV=0 / noise ~1e-12
#   2) Bisect / step toward first GC spike (5.525 for gids 500006/500009)
#   3) phases=1 at first bad tstop → post_setup / post_solve / pre_nonvint / post_nonvint
#   4) rdcellstate field diffs (na8st, CadepK, V, matrix, …)
#
# Note: prcellstate uses morph-stable cell-local inodes (BFS by secname+segi) so
# CPU (no permute) vs native GPU (permute 2) compare the same compartments.
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

# Avoid comparing stale dumps from earlier tstops (ls *t* is not unique).
rm -f "${gid}"_cpu-t*.nrndat "${gid}"_gpu-t*.nrndat \
  "${gid}"_cpu_t*.nrndat "${gid}"_gpu_t*.nrndat \
  "${gid}"_*_post_setup.nrndat "${gid}"_*_post_solve.nrndat \
  "${gid}"_*_pre_nonvint.nrndat "${gid}"_*_post_nonvint.nrndat 2>/dev/null || true
rm -f main_prcs.hoc 2>/dev/null || true

echo "=== dentate prcellstate gid=$gid tstop=$tstop phases=$phases ==="
run_one 0 cpu
run_one 1 gpu

# Resolve dump names written by this run (HOC %g may drop .0).
# Prefer newest mtime among tag-matching files.
cpu="$(ls -1t ${gid}_cpu-t*.nrndat 2>/dev/null | head -1 || true)"
gpu="$(ls -1t ${gid}_gpu-t*.nrndat 2>/dev/null | head -1 || true)"
if [[ -z ${cpu:-} || -z ${gpu:-} ]]; then
  echo "error: missing dumps for gid=$gid (cpu='$cpu' gpu='$gpu')" >&2
  ls -la ./*"${gid}"* 2>/dev/null || true
  exit 1
fi

echo "=== rdcellstate $cpu vs $gpu ==="
python3 "$rdcellstate" --ignore-unused "$cpu" "$gpu" || true
echo "exit note: rdcellstate exit 1 on any field noise is normal; inspect Top diffs"

# If phase dumps exist, compare each phase (first-diff focus).
if ls "${gid}"_*_post_setup.nrndat >/dev/null 2>&1; then
  echo "=== phase compares (cpu_* vs gpu_*) ==="
  for phase in post_setup post_solve pre_nonvint post_nonvint; do
    c="$(ls -1t ${gid}_cpu_t*_${phase}.nrndat 2>/dev/null | head -1 || true)"
    g="$(ls -1t ${gid}_gpu_t*_${phase}.nrndat 2>/dev/null | head -1 || true)"
    if [[ -n ${c:-} && -n ${g:-} ]]; then
      echo "--- phase $phase: $c vs $g ---"
      python3 "$rdcellstate" --ignore-unused "$c" "$g" 2>&1 | head -25 || true
    fi
  done
fi
