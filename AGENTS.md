# Agent rules — `nrngpu` native GPU qualification

Full handoff and starting prompt: **`GROK-GPU-NATIVE.md`** (read on new sessions).

## Workspace

- Primary repo: `~/neuron/nrngpu` (this tree). Commit here.
- Branch: `local/gpu-native-qualification`.
- Do not treat `~/neuron/core-neuron-gpu` as the git workspace (salvage/notes only).

## Build and env

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_NATIVE_GPU_DEVICE_NONVINT=1
export NRN_GPU_BACKEND_TEST=native
export NRN_GPU_PERMUTE=2
```

Build: `~/neuron/nrngpu/build-gpu` (`./grok-bld` or `ninja` / `ninja install`).

## GPU test harness

```bash
cd ~/neuron/nrngpu/build-gpu/test/external_ringtest/neuron_gpu_native_mpi
./prcellstate_native_gpu.sh 32 1
```

Use **full shell permissions** for GPU/OpenACC/MPI. Confirm GPU with `nvidia-smi -L`
and `Info : 1 GPUs shared by 1 ranks per node` in `special` output.

## Design constraints

- **Full GPU fixed steps** for ringtest (CoreNEURON-style). No per-step host
  post-solve voltage path on the hot path (no `vec_rhs` pull → host `nrn_update_voltage`
  → push `vec_v` as the primary fix).
- Stage 1 NetReceiveBuffer is committed; runtime inactive until Stage 2+ registrations.
- Long-term gate: 688 spikes @ `tstop=100`, gid 32 CPU parity via `rdcellstate`.

## Execute, don’t delegate

Run commands yourself (`prcellstate_native_gpu.sh`, `ninja`, `rdcellstate`). Do not tell the user
what to run unless blocked (e.g. no GPU in environment after `nvidia-smi` fails).