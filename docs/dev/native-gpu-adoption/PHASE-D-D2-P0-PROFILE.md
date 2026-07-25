# Phase D2-P0 — CUDA solver launcher profile (Traub `use_gap=0`)

Branch: `hines-grok/feature/neuron-native-gpu-phase-d-performance` @ `8596d2eaa`  
Model: `~/models/82894`, 356 cells, `dt=0.025`, `nthread=1`, `OMP_NUM_THREADS=1`  
Host: ThinkStation P5, NVIDIA T1000 8GB, NVHPC 25.9

Raw artifacts: `~/models/82894/results/d2-p0/`

---

## 1. Wall-clock reference (`mytstop=100` ms)

| Case | Runtime (s) | vs CoreNEURON GPU | Spikes | vs D1 native |
|------|------------:|------------------:|-------:|-------------:|
| `neuron_gpu_native` | **58.1** | **5.2×** slower | 4474 ✓ | **2.1× faster** |
| `neuron_cpu` | 53.5 | — | 4474 ✓ | — |
| `coreneuron_gpu` | 11.1 | 1.00× | 4474 ✓ | — |

Short run (`mytstop=10` ms): native **15.4 s** (D1: 20.6 s), CoreNEURON GPU **~2.2 s** (D1).

---

## 2. Layer 1 — phase timer (native, 100 ms)

Tracked total **54.3 s** (BENCHMARK **~63 s** with phase timer enabled; **~58 s** in quiet benchmark).

| Phase | D2-P0 (s) | D2-P0 % | D1 (s) | D1 % | Δ |
|-------|----------:|--------:|-------:|-----:|---|
| **lastpart** | 24.0 | **44.2%** | 24.8 | 20.7% | now dominant |
| **setup-tree-matrix** | 15.3 | **28.3%** | 16.0 | 13.4% | unchanged |
| **post-solve** | 9.4 | **17.3%** | 10.3 | 8.6% | unchanged |
| **matrix-solver** | **2.1** | **3.9%** | 66.7 | 55.8% | **−97%** |
| deliver-events | 2.2 | 4.0% | 0.1 | 0.1% | MPI/timer overhead |
| matrix-sync | 0.85 | 1.6% | 0.85 | 0.7% | — |
| vecplay-sync | 0.48 | 0.9% | 0.53 | 0.4% | — |

**Takeaway:** D2-P0 eliminated the solver bucket. Remaining gap vs CoreNEURON GPU is now
**lastpart (host nonvint/state)**, **setup-tree-matrix**, and **host post-solve** — matching
the revised D1 priority order after P6.

---

## 3. Layer 2 — Nsight (`mytstop=10` ms, 400 steps)

Report: `results/d2-p0/nsys_native.nsys-rep`

| Kernel | GPU time | Instances | Avg / step | D1 avg / step |
|--------|----------:|----------:|-----------:|--------------:|
| `solve_interleaved2_kernel_ptrs` | 199 ms | 400 | **0.50 ms** | 16.8 ms (OpenACC) |
| `nrn_rhs_*` | 8.9 ms | 400 | 22 µs | 18 µs |
| `nrn_lhs_*` | 6.4 ms | 400 | 16 µs | 13 µs |

CUDA solver kernel time is now **on par with CoreNEURON** (`solve_interleaved1` ~0.52 ms/step
in D1). OpenACC `Wait@cellorder.cpp` is no longer the top hotspot.

---

## 4. CTest gates

| Test | Result |
|------|--------|
| `external_ringtest::neuron_gpu_native_mpi` | **Pass** (688 spikes) |
| `external_ringtest::neuron_gpu_native_mpi_gap` | **Fail** (SIGSEGV rank 1; pre-existing gap path) |
| `external_ringtest::compare_neuron_gpu_native_mpi_gap` | Pass (stale compare script) |

Traub `use_gap=0` gate: **4474 spikes** via `run_benchmark.py`.

---

## 5. Revised priority (post D2-P0)

| Priority | Target | Est. Traub impact |
|----------|--------|-------------------|
| **P1 → D4** | Device `lastpart` / nonvint | ~24 s (44% tracked) |
| **P2 → D2** | Device post-solve (drop host `secondorder=2` fallback) | ~9 s (17%) |
| **P3 → D5** | Matrix setup sync reduction | ~15 s (28%) |
| P4 → D3 | Download / `gpu_download_flush` policy | small (flush removed from host path) |
| P5 → D6 | Traub sign-off vs CoreNEURON | 58 s → target ≤22 s (2× CN) |

**Recommended next package:** **D4** (device lastpart) — largest remaining bucket and aligns
with CoreNEURON running mechanism `state`/`cur` on GPU each step.