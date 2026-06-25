# Phase C — CoreNEURON runtime parity (native GPU)

Development journal entry for Phase C on branch
`hines-grok/feature/neuron-native-gpu-phase-c`.

Published scope contract: [`native-gpu-phase-c.rst`](../native-gpu-phase-c.rst).

## Goal

Extend native GPU (`gpu.backend="native"`, `coreneuron.enable=False`) so fixed-step
models with **MPI**, **multi-thread**, **gap junctions**, and **LFP/fast_imem**
workloads match **CoreNEURON GPU runtime capability** — not necessarily identical
implementation or performance.

Phase B tip (`hines-grok/feature/neuron-core-gpu-adoption`) is the parent branch.

## Why Phase C

| Phase B limit | CoreNEURON GPU today | Phase C target |
|---------------|----------------------|----------------|
| Full CPU fallback when `setup_transfer` | GPU integrate + hybrid partrans | Device gather/scatter (C1) |
| 19 single-process modtests only | MPI `*_py_gpu` tests | Native MPI modtests (C2) |
| `pc.nthread(n>1)` warning only | Production multi-thread | Validated threading (C3) |
| No native LFP path | CPU LFP from membrane current | GPU `fast_imem` + CPU LFP (C4) |

## Work packages

| ID | Summary | Depends on |
|----|---------|------------|
| C0 | Branch + scope doc | Phase B tip |
| C1 | Device partrans; remove gap CPU fallback | C0 |
| C2 | MPI + gap CTest expansion | C1 |
| C3 | Multi-thread OpenACC | C1 |
| C4 | LFP / fast_imem | C1 |
| C5 | Traub gap benchmark + perf | C2, C3, C4 |

## C0 status

- [x] Branch `hines-grok/feature/neuron-native-gpu-phase-c`
- [x] Sphinx scope contract `docs/dev/native-gpu-phase-c.rst`
- [x] This journal entry

## C1 status

- [x] Remove gap CPU fixed-step fallback (`fadvance.cpp`)
- [x] Align GPU gap step with CPU parent dispatch (mpi + lastpart in `nrn_fixed_step`)
- [x] Device gather for MPI `outsrc_buf_` voltage sources (`neuron/gpu/partrans.cpp`)
- [x] Host scatter + `update device` for partrans targets (`thread_transfer`)
- [x] C2 MPI modtest expansion (`spikes_mpi*_py_gpu_native`, `test_subworlds_py_gpu_native`, ringtest `-gap` MPI)

## C3 status

- [x] Per-thread device gather for MPI `outsrc_buf_` when `pc.nthread(n>1)`
- [x] Per-thread CUDA streams in multithread gather and `insrc_buf_` upload
- [x] Remove Phase B multithread warning once `test_natrans_py_gpu_native` is in the modtest set

## C4 status

- [x] GPU `fast_imem` in `post_solve_on_device` (no full-step host fallback for plain gaps)
- [x] Extracellular `v+vext` gap sources: host `nrnthread_vi_compute_` after device post-solve
- [x] `fast_imem_py_gpu_native` remains the primary `i_membrane_` parity check

## C5 status

- [x] Skip per-step matrix host roundtrip when rhs/d stay on device (`matrix_rhs_d_stays_on_device_for_solve`)
- [x] Traub 1/10 `use_gap=1`: **7873 spikes** match `neuron_cpu` on GPU integration path (no CPU fixed-step fallback)
- [x] Traub benchmark table recorded below; CoreNEURON gap raster delta (−6 spikes) unchanged from Phase B

### Traub sign-off (ModelDB 82894, `build-gpu-grok` install, T1000, OMP=1)

Branch tip at benchmark: `hines-grok/feature/neuron-native-gpu-phase-c` (post-C4 + C5 matrix sync).

| Config | Case | Runtime (s) | vs CPU | Spikes | Match CPU |
|--------|------|------------:|-------:|-------:|:---------:|
| `use_gap=0` | `neuron_cpu` | 53.2 | 1.00× | 4474 | ref |
| `use_gap=0` | `neuron_gpu_native` | 133.7 | 0.40× | 4474 | yes |
| `use_gap=1` | `neuron_cpu` | 56.5 | 1.00× | 7873 | ref |
| `use_gap=1` | `neuron_gpu_native` | 179.0 | 0.32× | 7873 | yes |

Phase C restores **correctness** on the GPU gap path (7873 vs 7873). Runtime is slower than
Phase B’s full CPU fallback (~1× CPU) because hybrid partrans and recording sync remain on
the host; CoreNEURON GPU (`use_gap=1`) is still ~4.4× faster than NEURON CPU on this host.

Harness: `~/models/82894/run_benchmark.py` with `x86_64` on `PATH` (see `BENCHMARK.md`).
Raw: `results/summary.txt`, `results/use_gap_1/summary.txt`.

## Phase C checklist

- [x] C0–C5 work packages on `hines-grok/feature/neuron-native-gpu-phase-c`
- [x] 21/21 `*_py_gpu_native` modtests (incl. MPI natives)
- [x] Gap single-rank + MPI ringtest `-gap`
- [x] Traub `use_gap=1` raster parity

## Key reference code

| Topic | CoreNEURON (reference) | NEURON native (today) |
|-------|------------------------|------------------------|
| Partrans GPU | `src/coreneuron/network/partrans.cpp` | `src/nrniv/partrans.cpp` (host pointers) |
| Gap fallback gate | N/A (no fallback) | `src/nrnoc/fadvance.cpp` |
| GPU step | `src/coreneuron/sim/fadvance_core.cpp` | `src/neuron/gpu/fadvance_gpu.cpp` |
| LFP | `src/coreneuron/io/lfp.cpp` | Not on native path |

## Sign-off commands (target end state)

```console
ctest --output-on-failure -R '_py_gpu_native'
ctest --output-on-failure -R 'gpu_device_assign_mpi'
# After C2:
ctest --output-on-failure -R 'spikes_mpi.*_py_gpu_native'
```

Manual: `test/gjtests/test_par_gj_native_gpu.py`; ringtest `-gap -gpu-native`;
Traub `use_gap=1` benchmark (ModelDB 82894).

See [05-future-work.md](05-future-work.md) for items moved from Phase B deferrals
into Phase C (device partrans, MPI modtests, multi-thread, LFP).
