#!/usr/bin/env bash
# Ensure NVIDIA CUDA Multi-Process Service (MPS) is running.
#
# Product policy for NEURON native GPU multi-rank on one GPU: start MPS once per
# node before multi-process OpenACC runs. Without MPS, dense OpenACC launch/wait
# traffic thrashs context switches (e.g. reduced_dentate 4-rank ~37 s vs ~3 s with
# MPS). CoreNEURON shares better without MPS; native needs it today.
#
# Idempotent. Does not stop MPS on exit (leave it up for subsequent product runs).
# Never fails the caller: missing control binary or start failure → warn and exit 0
# so correctness ctests still run (just slower).
#
# Usage:
#   source-or-call before mpiexec multi-rank native GPU:
#     bash path/to/ensure_cuda_mps.sh
#   Env:
#     CUDA_MPS_PIPE_DIRECTORY  (default /tmp/nvidia-mps) — same as runtime warn
#     NRN_SKIP_CUDA_MPS=1      — no-op (opt out)
#     NRN_CUDA_MPS_QUIET=1     — suppress "already active" / "started" lines
#
# See: doc/gpu/native-coreneuron-parity.md (multi-rank), docs/dev/native-gpu-build.rst

set -euo pipefail

if [[ "${NRN_SKIP_CUDA_MPS:-0}" == "1" ]]; then
  exit 0
fi

pipe_dir="${CUDA_MPS_PIPE_DIRECTORY:-/tmp/nvidia-mps}"
control_sock="${pipe_dir}/control"

_log() {
  if [[ "${NRN_CUDA_MPS_QUIET:-0}" != "1" ]]; then
    echo "$*"
  fi
}

_warn() {
  echo "$*" >&2
}

mps_control_socket_present() {
  # Matches neuron::gpu::cuda_mps_likely_active() in device_assign.cpp
  [[ -e "${control_sock}" ]]
}

if mps_control_socket_present; then
  _log "Info: CUDA MPS already active (${control_sock})"
  exit 0
fi

if ! command -v nvidia-cuda-mps-control >/dev/null 2>&1; then
  _warn "Warning: nvidia-cuda-mps-control not found on PATH."
  _warn "  Native GPU multi-rank on one device is often 10×+ slower without MPS."
  _warn "  Install the NVIDIA driver MPS tools or set NRN_SKIP_CUDA_MPS=1 to silence."
  exit 0
fi

# Start daemon (may print log-dir warnings on workstations without /var/log/nvidia-mps)
if ! nvidia-cuda-mps-control -d >/dev/null 2>&1; then
  # Some stacks print to stderr but still start; re-check socket.
  nvidia-cuda-mps-control -d 2>/dev/null || true
fi

# Wait briefly for the control socket
for _ in $(seq 1 50); do
  if mps_control_socket_present; then
    _log "Info: started CUDA MPS (nvidia-cuda-mps-control -d)"
    exit 0
  fi
  sleep 0.1
done

_warn "Warning: failed to start CUDA MPS (no ${control_sock} after nvidia-cuda-mps-control -d)."
_warn "  Multi-rank native GPU may thrash. Manual: nvidia-cuda-mps-control -d"
_warn "  Stop later: echo quit | nvidia-cuda-mps-control"
exit 0
