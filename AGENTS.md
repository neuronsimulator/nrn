# Agent rules — native GPU on heap-free network SoA

Integration branch: **`local/gpu-native-net-soa`** = `local/cpu-net-soa-heap-free`
(PR #3826) + `local/gpu-native-qualification`.

| Doc | Role |
|-----|------|
| **`GROK-GPU-NATIVE.md`** | GPU qualification handoff (read on new sessions) |
| **`doc/gpu/native-coreneuron-parity.md`** | Ordered next work: P0 triage → P1+ CoreNEURON feature matrix; session rename rules |
| **`doc/gpu/native-partrans.md`** | Native gap / source→target design (buffer path first) |
| **`GROK-NETWORK-SOA.md`** | CPU network SoA / heap-free charter |
| **`doc/network-soa/heap-free.md`** | Weight-index ABI, packing A |
| **`~/neuron/notes/PORTFOLIO.md`** | Cross-project map (feature vs platform); do not expand scope into other rows without user intent |
| **`~/neuron/notes/META-ORG.md`** | Session/succession process (parked; not for mid-GPU digressions) |

## Workspace

- Primary tree: `~/neuron/nrngpu` (this tree). Commit here.
- Living tip: **`local/gpu-native`** (alias of former `local/gpu-native-net-soa`).
  Exploratory perf: side branches e.g. `local/gpu-p4-gap-phase-ab` — cherry-pick
  into tip only what proves useful (do not auto-ff speculative instrumentation).
- Do not treat `~/neuron/core-neuron-gpu` as the git workspace (salvage only).
- CPU-only SoA worktree (optional): `~/neuron/cpu_net_soa`.

## Build and env (GPU)

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_GPU_BACKEND_TEST=native
export NRN_GPU_PERMUTE=2
# Device nonvint is mandatory under native (no NONVINT env; Gate C fail → error).
```

Build: `~/neuron/nrngpu/build-gpu` (`./grok-bld` or `ninja` / `ninja install`).

## GPU test harness

```bash
cd ~/neuron/nrngpu/build-gpu/test/external_ringtest/neuron_gpu_native_mpi
./prcellstate_native_gpu.sh 32 1
```

Use **full shell permissions** for GPU/OpenACC/MPI. Confirm GPU with `nvidia-smi -L`.

**Multi-rank on one GPU:** native OpenACC multi-process without CUDA MPS is often
**10×+ slower** (dentate 4-rank ~37 s vs ~3 s with MPS). For multi-rank product:

```bash
nvidia-cuda-mps-control -d   # once per node; stop: echo quit | nvidia-cuda-mps-control
```

Expect `Info : 1 GPUs shared by N ranks per node` (N = local ranks). CN shares
better without MPS; native needs MPS for multi-rank-on-1-GPU today.

## Design constraints

- **High performance is sacred.** CoreNEURON is a **guide** (full step on GPU, little
  CPU traffic during a step — spikes + optional trajectories), **not law**. Better-
  performing algorithms than CoreNEURON are valid candidates. See
  **`GROK-GPU-NATIVE.md` → Permanent: high performance is sacred**.
  Trajectory reference: `trajectory` in `src/nrncvode/netcvode.cpp`; device gather
  in `src/coreneuron/sim/fadvance_core.cpp` (`nrncore2nrn_send_values`). Avoid full
  SoA pull as the default for `Vector.record` / lastpart unless measured.
- No per-step host post-solve voltage path as the primary fix (`vec_rhs` pull →
  host `nrn_update_voltage` → push `vec_v`).
- **All threads on device** under native: if a thread is on GPU, it is entirely on
  GPU for the step (except spikes + sparse source→target buffers). Kernel host
  fallbacks → hard error once qualified. No bulk mech SoA H→D mid-psolve as product.
  Mixed CPU/GPU threads only if free. See **`GROK-GPU-NATIVE.md` → thread residency**.
- **Heap-free network:** sim-path identity is `weight_index` only (PR #3826). Do not
  reintroduce `NetCon::weight_` heap or Stage-2 `_receive_weight` shims.
- NetReceiveBuffer + device `net_buf_receive` for all buffered NET_RECEIVE (incl.
  net_send → NetSendBuffer indices + host deliver with heap-free weight_index).
  No host apply of NET_RECEIVE body on the native-GPU path.
- Long-term gate: 688 spikes @ `tstop=100`, gid 32 CPU parity via `rdcellstate`.

## Execute, don’t delegate

Run commands yourself (`prcellstate_native_gpu.sh`, `ninja`, `rdcellstate`). Do not tell
the user what to run unless blocked (e.g. no GPU after `nvidia-smi` fails).

## Session end: commit, do not push

After a coherent step: **commit locally** with a clear message. **Do not `git push`**
unless the user asks. User reviews with `git show HEAD` and either pushes or requests
amend / follow-up. Same workflow preference for all sessions on this machine
(`PORTFOLIO.md` / `META-ORG.md`).
