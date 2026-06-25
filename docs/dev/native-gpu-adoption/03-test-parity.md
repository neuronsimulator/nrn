# Test parity

## Native modtest suite

Phase B verification anchor: **19** CTest targets matching `*_py_gpu_native`.

They mirror single-process CoreNEURON `*_py_gpu` modtests with:

```text
NRN_GPU_BACKEND_TEST=native
NRN_GPU_PERMUTE=2
OMP_NUM_THREADS=1
```

Defined in `test/CMakeLists.txt` (G4 native GPU modtest parity block).

Quick check:

```console
ctest -N -R '_py_gpu_native'
ctest --output-on-failure -R '_py_gpu_native'
```

## `_py_gpu` vs `_py_gpu_native`

| Aspect | `*_py_gpu` | `*_py_gpu_native` |
|--------|------------|-------------------|
| Backend | CoreNEURON embedded GPU | `gpu.backend=native` |
| `coreneuron.enable` | True (typical) | False |
| Mechanism codegen | CoreNEURON OpenACC | NEURON native OpenACC (NMODL visitor) |
| Spike / MPI model | CoreNEURON | Native (CPU queues + GPU integration) |
| Reference | NEURON CPU | Same NEURON CPU reference |
| Count | ~40 single-process variants | 19 (Phase B parity set) |

Parity means: same mod file, same CPU reference output, native backend
matches within test tolerances.

## Broader GPU regression

Full GPU CI (CoreNEURON + native + ringtest) uses:

```console
ctest --output-on-failure -R gpu
```

Expect **~96** tests when MPI + CoreNEURON GPU + native are all enabled
(authoritative count: `ctest -N -R gpu` in your build tree).

Native-only developers can rely on `_py_gpu_native` + `testneuron_gpu_*` unit
tests without running the full CoreNEURON GPU matrix.

## Unit tests (`test/unit_tests/gpu/`)

| Target | Exercises |
|--------|-----------|
| `testneuron_gpu_config` | `set_enable`, `set_backend`, guards |
| `testneuron_gpu_device_state` | Device lifecycle |
| `testneuron_gpu_upload` | Upload path |
| `testneuron_gpu_fadvance` | Timestep dispatch stubs |
| `testneuron_gpu_net_events` | Net event host stages |
| `testneuron_gpu_offload` | OpenACC offload helpers |
| `testneuron_gpu_device_assign` | MPI device assignment |

Filter: `ctest -R testneuron_gpu`

## Gap junction supplementary test

Not part of the 19-ctest parity set; run manually after building with GPU:

```console
cd test/gjtests && nrnivmodl
python3 test_par_gj_native_gpu.py
```

Compares `test_par_gj` dendrite voltages CPU vs `gpu.backend=native`. Ringtest
`-gap -gpu-native` (single rank) is an additional raster check; CTest
`neuron_gpu_native_mpi` does not pass `-gap` today.

## Ringtest native benchmark

External ringtest supports `-gpu-native` (orthogonal to CoreNEURON `-gpu`).
CTest: `external_ringtest::neuron_gpu_native_mpi`. See `gpu-testing.rst`.

## CTest stability notes (Part A)

- Pytest modtests need `LD_LIBRARY_PATH` including the build `lib` dir (NVHPC
  `pgcudafat` loader).
- `libnrnmech` relinks when `libnrniv.so` changes to avoid stale ACC object
  paths.

Documented in Part A commit `b5fdf7a79`.