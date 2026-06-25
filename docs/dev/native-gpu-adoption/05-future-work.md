# Future work (explicitly out of Phase B)

Items below were considered during Phase B planning and **intentionally deferred**
to avoid mission creep. Phase B closes with PR7; these are not blockers for the
Phase B checklist.

## Runtime features

| Item | Notes |
|------|-------|
| CVode / IDA on native GPU | Separate project; different timestep contract |
| GPU spike priority queue | Requires MPI + device ordering design |
| Full multi-thread OpenACC | Harden per-thread CUDA contexts; remove warning |
| `sparse13` / extracellular / LFP on device | Eliminate per-step host fallbacks |
| Device partrans gather/scatter | Remove gap CPU fixed-step fallback; host/MPI only when `nrnmpi_numprocs > 1` |
| MPI-native modtest expansion | Beyond 19-test single-process parity |
| Per-step matrix host sync elimination | Performance optimization in `treeset.cpp` |

## Build / install

| Item | Notes |
|------|-------|
| `-DNRN_ENABLE_GPU=ON` without CoreNEURON | L0 decouple; OpenACC flags without `add_subdirectory(coreneuron)` |
| Native-only CI slice | Smaller `ctest` profile for minimal GPU installs |

## Documentation / tooling (optional)

| Item | Notes |
|------|-------|
| CodeTour / `@native-gpu` anchors | IDE-clickable map of L3–L7 |
| Compiler warnings cleanup | `src/neuron/gpu/` NVHPC pragma noise |
| Expanded C++ unit tests | Cover download flush, fallback matrix |

Phase C scope for the first four runtime rows above is defined in
[`native-gpu-phase-c.rst`](../native-gpu-phase-c.rst). Track remaining rows as
GitHub issues when opening post–Phase C work.
