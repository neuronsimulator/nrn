# Phase B complete

Checklist for closing native GPU **fixed-step** Phase B on branch
`hines-grok/feature/neuron-core-gpu-adoption`.

## Runtime contract

- [x] Fixed-step `pc.psolve` on `gpu.backend=native` without `coreneuron.enable`
- [x] OpenACC matrix solve + mechanism state + post-solve on device
- [x] CPU spike queues + MPI; GPU `NET_RECEIVE` and `net_send` buffer
- [x] Gap junctions with device voltage sync before MPI
- [x] Batch download API (`download_flush_interval`)
- [x] CVode rejected when native GPU enabled
- [x] Threading warning for `pc.nthread(n>1)`

## Verification

- [x] 19/19 `*_py_gpu_native` modtests pass
- [x] `ctest -R gpu` stable on NVHPC dev build (Part A harness fixes)
- [x] Unit tests: `ctest -R testneuron_gpu`

## Documentation

- [x] PR6 — Sphinx scope contract (`native-gpu-fixed-step.rst`)
- [x] PR7 — This development journal (`native-gpu-adoption/*.md`)
- [x] Build/runtime tables explicit in Sphinx (CMake vs `gpu.*` / `coreneuron.enable`)

## Code boundary PRs (Part A)

| Commit | Summary |
|--------|---------|
| `ff48b5e78` | CVode rejection |
| `b5fdf7a79` | CTest / `libnrnmech` stability |
| `e800b0ffc` | Thread + spike/`NET_RECEIVE` policy docs |

## Feature PRs (core path)

| Commit | Summary |
|--------|---------|
| `b6bad25dd` … `e2eddbf30` | Matrix, mechanisms, post-solve, download, modtest parity |

## Explicit deferrals (not Phase B)

- Standalone `-DNRN_ENABLE_GPU=ON` configure (CoreNEURON=OFF)
- CVode on GPU, GPU spike queue, full multi-thread OpenACC
- PR4 warnings cleanup, PR5 unit-test expansion, CodeTour (optional)

See [05-future-work.md](05-future-work.md).

## Sign-off commands

```console
ctest --output-on-failure -R '_py_gpu_native'
ctest --output-on-failure -R testneuron_gpu
```

Phase B journal last updated at commit cited in [appendix-commits.md](appendix-commits.md).