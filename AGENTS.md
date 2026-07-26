# Agent rules — native GPU on heap-free network SoA

Integration branch: **`local/gpu-native-net-soa`** = `local/cpu-net-soa-heap-free`
(PR #3826) + `local/gpu-native-qualification`.

| Doc | Role |
|-----|------|
| **`GROK-GPU-NATIVE.md`** | GPU qualification handoff (read on new sessions) |
| **`GROK-NETWORK-SOA.md`** | CPU network SoA / heap-free charter |
| **`doc/network-soa/heap-free.md`** | Weight-index ABI, packing A |
| **`~/neuron/notes/PORTFOLIO.md`** | Cross-project map (feature vs platform); do not expand scope into other rows without user intent |
| **`~/neuron/notes/META-ORG.md`** | Session/succession process (parked; not for mid-GPU digressions) |

## Workspace

- Primary tree: `~/neuron/nrngpu` (this tree). Commit here.
- Branch: `local/gpu-native-net-soa`.
- Do not treat `~/neuron/core-neuron-gpu` as the git workspace (salvage only).
- CPU-only SoA worktree (optional): `~/neuron/cpu_net_soa`.

## Build and env (GPU)

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
- **Heap-free network:** sim-path identity is `weight_index` only (PR #3826). Do not
  reintroduce `NetCon::weight_` heap or Stage-2 `_receive_weight` shims.
- Stage 1 NetReceiveBuffer is present; runtime inactive until Stage 2+ registrations
  use Weight SoA indices.
- Long-term gate: 688 spikes @ `tstop=100`, gid 32 CPU parity via `rdcellstate`.

## Execute, don’t delegate

Run commands yourself (`prcellstate_native_gpu.sh`, `ninja`, `rdcellstate`). Do not tell
the user what to run unless blocked (e.g. no GPU after `nvidia-smi` fails).
