# Design decisions

Rationale for choices that are not obvious from the Sphinx scope contract alone.

## Spike delivery stays on CPU; `NET_RECEIVE` runs on GPU

**Decision:** Cross-rank NetCon scheduling uses CPU priority queues and MPI
exchange at the minimum delay interval. `NET_RECEIVE` mechanism code and
integration run on GPU during the fixed step.

**Why:** Matches the CoreNEURON split already proven at scale. A GPU-resident
spike queue would require new ordering guarantees across MPI ranks and
device/host event ordering. Phase B reuses the host network stack.

**Artifacts:** `net_events.cpp`, `net_events.hpp`, `net_send_buffer.cpp`,
`deliver_net_events` / `deliver_post_step_events_host` in `fadvance_gpu.cpp`.

## `net_send` / threshold: GPU buffer, CPU/MPI for cross-cell

**Decision:** Presynaptic `net_send` and threshold crossings generated on device
are buffered (`NetSendBuffer`). Self-events may flush on device; cross-cell
outputs go to CPU/MPI scheduling.

**Why:** Preserves existing NetCon semantics without a device-side priority
queue. Buffer flush points are tied to the fixed-step cadence.

## Memb_list padding for device upload

**Decision:** Upload padded `Memb_list` shells so OpenACC mechanism kernels see
a uniform SOA layout on device.

**Why:** Host `Memb_list` layout includes holes and permutations that do not map
1:1 to GPU warp-friendly access. Padding mirrors CoreNEURON's device layout
assumptions.

**Artifacts:** `upload_mechanisms.cpp`, `upload.cpp`.

## `ensure_cached` / device_token lifecycle

**Decision:** Device state uploads once per sorted model token; concurrent
upload paths guard with caching (regression fix for double-free in gap/natrans
tests).

**Why:** Re-uploading on every step would dominate cost; stale device state
after topology changes must invalidate cleanly.

**Artifacts:** `device_state.cpp`, `upload.cpp`.

## Gap junctions: three-phase partrans, host staging in Phase B

**Decision:** Gap transfer follows CPU partrans phases when
`pc.setup_transfer()` is active (not only when MPI or `nthread>1`):

1. Gather sources into buffers (`mpi_transfer` / `outsrc_buf_`)
2. `MPI_Alltoallv` when `nrnmpi_numprocs > 1` (skipped on one rank)
3. Scatter to targets (`thread_transfer` in `nonvint`)

Native GPU pulls source voltages device → host (`sync_gap_after_voltage_update`)
before gather so `mpi_transfer` reads host memory. Scatter remains CPU in lastpart.

**Why:** Partrans predates device-resident gap buffers. GPU gather/scatter without
host staging — especially when MPI is absent — is a future optimization, not
Phase B.

**Artifacts:** `partrans.cpp`, `sync.cpp`, `fadvance_gpu.cpp`.

## Batch download for recording

**Decision:** `download_flush_interval` controls how often post-solve state is
pulled to host for `Vector.record` and HOC graph reads. Default `1` (every
step); `0` defers to end of `psolve`.

**Why:** Per-step host pulls are correct but expensive. Batch API lets models
trade accuracy of mid-solve visibility for throughput.

**Artifacts:** `download.cpp`, `post_solve.cpp`, `neuron.gpu.download_flush_interval`.

## CVode rejected at enable time

**Decision:** `native_gpu_configuration_error()` blocks native GPU when CVode is
active (Python `gpu.enable`, HOC `gpu_enable`, entry to `nrn_fixed_step`).

**Why:** Variable-step integration is out of Phase B scope; silent fallback to
CPU would hide misconfiguration.

**Artifacts:** `config.cpp`, `gpu.py`, `ocbbs.cpp`, `fadvance.cpp`.

## Threading: warn, do not block

**Decision:** `pc.nthread(n>1)` with native GPU emits a one-time warning; modtests
run with `OMP_NUM_THREADS=1` and `pc.nthread(1)`.

**Why:** Per-thread OpenACC CUDA contexts are fragile on some platforms. Blocking
would break existing scripts; warning sets expectations until a later hardening
pass.

**Artifacts:** `warn_native_gpu_multithread_policy()` in `config.cpp`,
`fadvance_gpu.cpp`.

## Subphase GPU path; tree solve vs sparse13

**Decision:** Native fixed-step is split into GPU subphases (matrix setup, tree
solve, post-solve, `NET_RECEIVE`, `net_send` buffer, …). ODE tree models use
**GPU tree solve** (`solve_interleaved`). DAE models (extracellular,
`LinearMechanism`) swap only the **matrix solve** to CPU `sparse13`
(`spFactor`/`spSolve`); other subphases are not architecturally excluded.

**Why:** Sparse Gaussian elimination is the missing GPU piece — a future phase,
like CVode-on-GPU. Phase B validates the ODE column only.

## Build still requires CoreNEURON at configure (Phase B)

**Decision:** `-DNRN_ENABLE_GPU=ON` currently requires `-DNRN_ENABLE_CORENEURON=ON`
at CMake configure. Runtime native path does not require `coreneuron.enable=True`.

**Why:** OpenACC flags, cellorder sources, and mechanism GPU codegen still flow
through the CoreNEURON configure path. Standalone GPU configure is explicitly
**out of Phase B** (see [05-future-work.md](05-future-work.md)).