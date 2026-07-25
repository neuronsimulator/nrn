# Native GPU adoption — overview (Phase B)

Development journal for NEURON's native fixed-step GPU path (`gpu.backend="native"`).
The published scope contract lives in Sphinx:
[`native-gpu-fixed-step.rst`](../native-gpu-fixed-step.rst).

## Goals (Phase B)

- Fixed-step `pc.psolve` / `h.fadvance` on the native GPU path without embedding
  CoreNEURON at **runtime** (`coreneuron.enable=False`).
- OpenACC matrix solve, mechanism integration, and post-solve on device.
- CPU spike priority queues + MPI at the minimum NetCon delay interval; GPU
  `NET_RECEIVE` computation and `net_send` buffering.
- Parity proof: **19/19** `*_py_gpu_native` modtests (mirror single-process
  `*_py_gpu` CoreNEURON modtests).
- Explicit boundaries: CVode rejected, documented threading and spike policy.

## Non-goals (deferred past Phase B)

See [04-known-limitations.md](04-known-limitations.md) and
[05-future-work.md](05-future-work.md). Phase B does **not** include:

- CVode / IDA on native GPU
- GPU spike priority queue
- Standalone `-DNRN_ENABLE_GPU=ON` configure without `-DNRN_ENABLE_CORENEURON=ON`
- Full multi-thread OpenACC maturity
- Device partrans gather/scatter (gap models use CPU fixed-step fallback today)
- MPI-native modtest expansion beyond the current parity set

## Stopping point

Phase B is **complete** when the runtime contract is coherent for fixed-step
`pc.psolve` (L0–L8; see [02-implementation-map.md](02-implementation-map.md))
and the checklist in [PHASE-B-COMPLETE.md](PHASE-B-COMPLETE.md) is satisfied.

Branch: `hines-grok/feature/neuron-core-gpu-adoption`  
Commits through PR7: `2087fb131` (appendix: [appendix-commits.md](appendix-commits.md)).

## Phase C (in progress)

Runtime parity with **CoreNEURON GPU** on `gpu.backend="native"` (MPI, threads,
device partrans for gaps, LFP/fast_imem). Branch:
`hines-grok/feature/neuron-native-gpu-phase-c`. Scope:
[`native-gpu-phase-c.rst`](../native-gpu-phase-c.rst),
journal: [PHASE-C-OVERVIEW.md](PHASE-C-OVERVIEW.md).

## Document map

| File | Purpose |
|------|---------|
| [01-design-decisions.md](01-design-decisions.md) | Why spike queues stay on CPU, Memb_list padding, etc. |
| [02-implementation-map.md](02-implementation-map.md) | L0–L8 layers → files |
| [03-test-parity.md](03-test-parity.md) | `_py_gpu` vs `_py_gpu_native` |
| [04-known-limitations.md](04-known-limitations.md) | Honest runtime limits |
| [05-future-work.md](05-future-work.md) | Deferred items (no Phase B scope creep) |
| [appendix-commits.md](appendix-commits.md) | One paragraph per adoption commit |
| [PHASE-B-COMPLETE.md](PHASE-B-COMPLETE.md) | Closure checklist |

## Conceptual levels (L0–L8)

Use these as lenses when reading code, tests, or PRs:

| Level | Name | Summary |
|-------|------|---------|
| L0 | Build | CMake, NVHPC, OpenACC; `NRN_ENABLE_GPU` |
| L1 | Runtime API | `neuron.gpu`, `ParallelContext` GPU methods |
| L2 | Device lifecycle | Upload once; invalidate on model change |
| L3 | Timestep dispatch | CPU vs native routing in `fadvance` |
| L4 | Integration step | Matrix, mechanisms, post-solve |
| L5 | Host fallbacks | sparse13, extracellular, LFP |
| L6 | Communication | Spike queues (CPU), gap MPI, `NetSendBuffer` |
| L7 | Recording | Batch download / flush interval |
| L8 | Verification | Modtests, unit tests, ringtest |

Phase B “done” means L0–L8 are coherent for fixed-step native `pc.psolve` with
the documented exclusions above.
