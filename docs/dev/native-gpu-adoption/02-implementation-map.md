# Implementation map (L0–L8)

File → responsibility guide for the native GPU path. Paths are relative to the
repo root.

## L0 — Build

| Artifact | Role |
|----------|------|
| `CMakeLists.txt` | `NRN_ENABLE_GPU`, CoreNEURON coupling guard |
| `src/neuron/CMakeLists.txt` | `neuron_gpu`, `neuron_gpu_upload` targets |
| `src/nrniv/CMakeLists.txt` | OpenACC cellorder sources linked into `libnrniv` |
| `cmake/coreneuron/OpenAccHelper.cmake` | NVHPC `-acc` compile/link flags |
| `cmake/neuronMechMaker.cmake` | OpenACC mechanism object builds |
| `docs/cmake_doc/options.rst` | User-facing CMake docs |

## L1 — Runtime API

| Artifact | Role |
|----------|------|
| `share/lib/python/neuron/gpu.py` | `neuron.gpu` module; CVode guard on enable |
| `share/lib/python/neuron/coreneuron.py` | Legacy `coreneuron.gpu` → `neuron.gpu` |
| `src/parallel/ocbbs.cpp` | HOC `gpu_enable`, `gpu_backend`, `gpu_device_count`, `gpu_download_flush_interval` |
| `src/neuron/gpu/config.cpp` | C++ runtime config; `native_gpu_configuration_error()` |

## L2 — Device lifecycle

| Artifact | Role |
|----------|------|
| `src/neuron/gpu/device_state.cpp` | Device resident state, invalidate |
| `src/neuron/gpu/upload.cpp` | SOA / tree / thread upload |
| `src/neuron/gpu/upload_mechanisms.cpp` | Padded `Memb_list` upload |
| `src/neuron/gpu/offload.cpp` | OpenACC/OpenMP offload helpers |

## L3 — Timestep dispatch

| Artifact | Role |
|----------|------|
| `src/nrnoc/fadvance.cpp` | `nrn_fixed_step` entry; native branch in `nrn_fixed_step_thread` |
| `src/neuron/gpu/fadvance_gpu.cpp` | `fixed_step_thread`, host event delivery hooks |
| `src/parallel/ocbbs.cpp` | `psolve` → `netpar_solve` vs `nrncore_psolve` |

## L4 — Integration step

| Artifact | Role |
|----------|------|
| `src/nrnoc/treeset.cpp` | OpenACC `setup_tree_matrix` path |
| `src/coreneuron/permute/cellorder*.cpp` | Interleaved solve (compiled into `libnrniv`) |
| `src/neuron/gpu/post_solve.cpp` | Device post-solve: voltage, fast_imem, capacity |
| `src/neuron/gpu/fadvance_gpu.cpp` | Orchestrates deliver → matrix → solve → post-solve |

## L5 — Host fallbacks

| Artifact | Role |
|----------|------|
| `src/neuron/gpu/post_solve.cpp` | `post_solve_needs_host_fallback()` |
| `src/nrnoc/fadvance.cpp` | CPU `nrn_update_voltage` when fallback required |

## L6 — Communication

| Artifact | Role |
|----------|------|
| `src/neuron/gpu/net_events.cpp` | Host-side net event stages |
| `src/neuron/gpu/net_events.hpp` | Spike / `NET_RECEIVE` policy comments |
| `src/neuron/gpu/net_send_buffer.cpp` | GPU `net_send` buffering |
| `src/neuron/gpu/net_send_buffer.hpp` | Buffer semantics |
| `src/neuron/gpu/sync.cpp` | Gap / voltage device↔host sync |
| `src/nrniv/partrans.cpp` | MPI gap transfer (host) |

## L7 — Recording

| Artifact | Role |
|----------|------|
| `src/neuron/gpu/download.cpp` | `batch_download_to_host`, flush interval |
| `src/neuron/gpu/download.hpp` | Download API |

## L8 — Verification

| Artifact | Role |
|----------|------|
| `test/CMakeLists.txt` | `*_py_gpu_native` modtests, GPU unit tests |
| `test/coreneuron/backend_helper.py` | Native vs CoreNEURON backend selection in tests |
| `test/unit_tests/gpu/*.cpp` | Catch2 GPU module tests |
| `test/external/ringtest/` | `-gpu-native` ringtest harness |
| `docs/dev/gpu-testing.rst` | CTest / ringtest workflows |
| `docs/dev/native-gpu-fixed-step.rst` | Phase B scope contract (PR6) |

## Dispatch flow (one line)

`pc.psolve` → (`coreneuron.enable` ? `nrncore_psolve` : `netpar_solve` →
`nrn_fixed_step` → (`gpu.enable && backend==native` ? `fadvance_gpu` : CPU thread
path)).