# Phase D1 — Baseline profile matrix (Traub `use_gap=0`)

Branch: `hines-grok/feature/neuron-native-gpu-phase-d-performance` @ `7d70d51a6`  
Model: `~/models/82894`, 356 cells, `dt=0.025`, `nthread=1`, `OMP_NUM_THREADS=1`  
Host: ThinkStation P5, NVIDIA T1000 8GB, NVHPC 25.9

Raw artifacts: `~/models/82894/results/d1/`

---

## 1. Wall-clock reference (`mytstop=100` ms)

| Case | Runtime (s) | vs CoreNEURON GPU | Spikes |
|------|------------:|------------------:|-------:|
| `neuron_gpu_native` | 122.9 | 12.3× slower | 4474 |
| `coreneuron_gpu` | 10.0 | 1.00× | 4474 |

Short run (`mytstop=10` ms, 400 steps): native **20.6 s**, CoreNEURON **2.2 s** (9.3×).

---

## 2. Layer 1 — `NRN_NATIVE_GPU_PHASE_TIMER=1` (native, 100 ms)

Tracked total **119.7 s** (BENCHMARK **123.3 s**; ~3 s untracked: spike exchange, MPI, psolve overhead).

| Phase | Wall (s) | % tracked | Calls/step hint |
|-------|---------:|----------:|----------------|
| **matrix-solver** | 66.7 | **55.8%** | 1× solve / step |
| **lastpart** | 24.8 | **20.7%** | host `nonvint` + record |
| **setup-tree-matrix** | 16.0 | **13.4%** | `nrn_rhs` + `nrn_lhs` |
| **post-solve** | 10.3 | **8.6%** | host fallback (`secondorder=2`) |
| matrix-sync | 0.85 | 0.7% | ~2× / step (8003 calls / 4000 steps) |
| vecplay-sync | 0.53 | 0.4% | voltage H↔D around VecPlay |
| deliver-events | 0.11 | 0.1% | net event delivery |
| download-flush | 0.35 | 0.3% | `gpu_download_flush=1` |

**Takeaway:** The gap is **not** primarily host↔device memcpy volume (Layer 1 shows &lt;2% in sync/download). Dominant buckets are **solver wall time**, **host lastpart**, **matrix setup**, and **host post-solve**.

---

## 3. Layer 2 — Nsight Systems (`mytstop=10` ms, 400 steps)

Reports: `results/d1/nsys_native.nsys-rep`, `results/d1/nsys_coreneuron.nsys-rep`

### 3a. CUDA kernels (top)

| Backend | Top kernel | GPU time | Instances | Avg / step |
|---------|------------|----------:|----------:|-----------:|
| Native | `neuron::solve_interleaved2_*` | 6.73 s | 400 | **16.8 ms** |
| Native | `nrn_rhs_*` | 7.0 ms | 400 | 18 µs |
| Native | `nrn_lhs_*` | 5.1 ms | 400 | 13 µs |
| CoreNEURON | `coreneuron::solve_interleaved1_*` | 206 ms | 400 | **0.52 ms** |
| CoreNEURON | `nrn_state_naf_*` + other `nrn_state_*` / `nrn_cur_*` | ~800 ms total | 400 each | 0.05–0.14 ms |

Native GPU spends **~99.8%** of tracked CUDA kernel time in a **single solve kernel** per step. CoreNEURON spreads work across solve (~25% of kernel time) plus **many mechanism state/current kernels** on device.

**Solver kernel gap:** ~**32×** per step (16.8 ms vs 0.52 ms) — exceeds the overall 12× wall-clock gap and points at the NEURON native `solve_interleaved2` launch/wait path (`Wait@cellorder.cpp:804` = 60.6% of OpenACC time on native).

### 3b. OpenACC waits / data motion (native)

| Region | % OpenACC time | Notes |
|--------|---------------:|-------|
| `Wait@cellorder.cpp:804` | 60.6% | Post-solve Hines launcher sync |
| `Wait@(OpenACC API)` | 24.1% | Runtime API waits |
| `Enqueue Upload` | 11.5% | Small H→D enqueues |

### 3c. CUDA memcpy (native)

| Operation | Count (10 ms) | Total time | Avg |
|-----------|--------------:|-----------:|----:|
| H→D | 432,563 | 331 ms | 0.8 µs |
| D→H | 800 | 23 ms | 29 µs |

High **H→D call count** (~1081/step) suggests many fine-grained unified-memory or `update device` operations; total memcpy time is still modest vs solver wait.

CoreNEURON OpenACC shows mechanism `Enter Data` / `Update` on mod files and `nrn_acc_manager` — state/current integrated on GPU each step. Native runs equivalent work in **lastpart** on host (20.7% wall).

---

## 4. Revised optimization priority (post-D1)

| Rank | Target | D1 evidence | Est. leverage |
|------|--------|-------------|---------------|
| **D2-P0** | **`solve_interleaved2` path** | 55.8% wall; 16.8 ms/step GPU wait; 32× vs CoreNEURON solve | **Critical** |
| **D2-P1** | **Device `lastpart` (state/current)** | 20.7% wall; CoreNEURON runs `nrn_state_*`/`nrn_cur_*` on GPU | High |
| **D2-P2** | **`setup-tree-matrix` GPU path** | 13.4% wall; `nrn_rhs`/`nrn_lhs` OpenACC + host orchestration | Medium |
| **D2-P3** | **Device post-solve** | 8.6% wall; host `second_order_cur` + `nrn_update_voltage` | Medium |
| D2-P4 | Matrix sync reduction | 0.7% wall in D1 (lower than pre-D1 estimate) | Low until P0–P1 done |
| D2-P5 | `gpu_download_flush` | 0.3% wall in D1 | Low on this benchmark |
| D2-P6 | Fine-grained H→D count | 432k memops / 400 steps — audit after P0 | TBD |

**Deferred:** `use_gap=1`, 6-spike CoreNEURON delta.

---

## 5. Recommended next steps (D2)

1. **Solver audit (P0):** Compare NEURON `solve_interleaved2_launcher` vs CoreNEURON `solve_interleaved1/2` — grid size, stream sync, cell grouping, SoA layout, and whether native waits on the host unnecessarily after each launch.
2. **Fuse device state (P1):** Port `nonvint` mechanism `state` + key `cur` paths to device as CoreNEURON does; keep ringtest + Traub 4474-spike gate.
3. **Re-profile** after each change with `NRN_NATIVE_GPU_PHASE_TIMER=1` and 10 ms Nsight slice.

---

## 6. Reproduce

```bash
# Layer 1 (100 ms native)
export NRN_NATIVE_GPU_PHASE_TIMER=1 OMP_NUM_THREADS=1
./x86_64/special -c mytstop=100 -c use_gap=0 -c enable_gpu=1 -c benchmark_quiet=1 \
  -c gpu_download_flush=1 init.hoc 2>results/d1/phase_timer_native.stderr

# Layer 2 (10 ms)
nsys profile -o results/d1/nsys_native --force-overwrite=true --trace=cuda,openacc \
  ./x86_64/special -c mytstop=10 -c use_gap=0 -c enable_gpu=1 -c benchmark_quiet=1 \
  -c gpu_download_flush=1 init.hoc

nsys stats --report cuda_gpu_kern_sum --format column results/d1/nsys_native.nsys-rep
nsys stats --report openacc_sum --format column results/d1/nsys_native.nsys-rep
```