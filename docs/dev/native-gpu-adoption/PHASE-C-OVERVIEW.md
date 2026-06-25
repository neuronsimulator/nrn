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
- [ ] C1 implementation (not started)

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
