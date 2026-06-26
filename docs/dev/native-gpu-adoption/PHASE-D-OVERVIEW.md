# Phase D — Performance parity (native GPU vs CoreNEURON GPU)

Development journal for Phase D on branch
`hines-grok/feature/neuron-native-gpu-phase-d-performance` (parent: Phase C tip).

## Goal

Close the runtime gap between **NEURON native GPU** (`enable_gpu=1`, `gpu.backend=native`,
`coreneuron=0`) and **CoreNEURON GPU** on the Traub benchmark (ModelDB 82894,
`~/models/82894`, `use_gap=0` first). Phase C delivered functional parity; Phase D
optimizes the hot path.

**Primary benchmarks**

| Harness | Config | Correctness gate |
|---------|--------|------------------|
| `run_benchmark.py -c config.yaml` | `use_gap=0`, 356 cells, 100 ms | 4474 spikes vs `neuron_cpu` |
| `ctest -R 'external_ringtest::'` | ringtest GPU native | existing CTest thresholds |
| (later) `config_use_gap_1.yaml` | gap + partrans | 7873 spikes vs `neuron_cpu` |

**Build / run:** `build-gpu-grok/grok-bld`, `nrnenv gpu-grok`, `nrnivmodl -coreneuron mod`.

## Baseline (`40310efbe`, ThinkStation P5, T1000, OMP=1)

| Case | `use_gap=0` runtime | Spikes | vs CoreNEURON GPU |
|------|--------------------:|-------:|------------------:|
| `neuron_cpu` | 52.3 s | 4474 | — |
| `neuron_gpu_native` | 124.7 s | 4474 | **12.9× slower** |
| `coreneuron_gpu` | 9.7 s | 4474 | 1.00× |

Native GPU is spike-correct but far from CoreNEURON GPU throughput. Gap junctions
(`use_gap=1`) deferred until the no-gap path is within striking distance.

**Known correctness debt (Phase D may address):** CoreNEURON reports 7867 vs NEURON 7873
spikes with gaps (−6); unrelated to native GPU but worth revisiting once perf work
stabilizes.

---

## Instrumentation strategy (three layers)

### Layer 1 — Custom phase timer (D0, default for iteration)

**Enable:** `export NRN_NATIVE_GPU_PHASE_TIMER=1`

Accumulates wall time per fixed-step bucket across `psolve`; prints a summary at
`finalize_psolve_download()` (end of `fadvance` / `psolve`).

| Phase key | What it measures |
|-----------|------------------|
| `deliver-events` | Host net event delivery before integrate |
| `vecplay-sync` | Voltage `update host/device` around VecPlay |
| `setup-tree-matrix` | `nrn_rhs` + `nrn_lhs` + axial assembly |
| `matrix-sync` | rhs/d host↔device OpenACC updates (count + time) |
| `matrix-solver` | `solve_interleaved` / Hines |
| `post-solve` | Host fallback or device `post_solve_on_device` |
| `download-flush` | `batch_download_post_solve` when `gpu_download_flush=1` |
| `lastpart` | `nonvint` state update + `AFTER_SOLVE` + record (no-gap path) |
| `gap-sync` | Voltage staging before gap gather (gap configs only) |

**Pros:** No rebuild; low overhead when disabled; aligns with NEURON fixed-step structure.  
**Cons:** Host-side only; does not attribute individual CUDA kernels inside mechanisms.

**Per-mechanism breakdown:** call `mech_time()` from HOC (see `ocbbs.cpp`) after zeroing
to populate `nrn_mech_wtime_` — `setup_tree_matrix` already instruments `cur-*` / `state-*`
via `Instrumentor` when Caliper is enabled.

Example Traub run:

```bash
export NRN_NATIVE_GPU_PHASE_TIMER=1
export OMP_NUM_THREADS=1
./x86_64/special -c mytstop=100 -c use_gap=0 -c enable_gpu=1 -c benchmark_quiet=1 init.hoc
# summary printed after psolve on stderr
```

Compare the same with `coreneuron=1 coreneuron_gpu=1 enable_gpu=0` (timer is native-GPU-only;
use Layer 2 for CoreNEURON).

### Layer 2 — Nsight Systems (GPU timeline, compare backends)

**Tool:** `nsys profile` (preferred; `nvprof` is deprecated).

Capture one rank, one thread, short run (e.g. `mytstop=10`):

```bash
nsys profile -o nrntraub_native --trace=cuda,nvtx,openacc \
  ./x86_64/special -c mytstop=10 -c use_gap=0 -c enable_gpu=1 -c benchmark_quiet=1 init.hoc
```

Repeat for CoreNEURON GPU. Diff:

- OpenACC `update` / `wait` density (host↔device transfers)
- `solve_interleaved2` kernel occupancy vs NEURON launch path
- Mechanism kernel launch count per timestep
- Idle gaps on GPU while host runs `deliver-events`, `lastpart`, post-solve

Optional: rebuild with `-DNRN_ENABLE_PROFILING=ON -DNRN_PROFILER=caliper` and
`CALI_CONFIG=nvtx` for NVTX markers on existing `Instrumentor::phase` regions
(`matrix-solver`, `cur-*`, `state-*`).

### Layer 3 — Existing `Instrumentor` / `NRN_PROFILE_REGIONS`

Already marks `timestep`, `matrix-solver`, `state-update`, per-mechanism `cur-*` /
`state-*`. Use when Caliper or LIKWID is configured (`docs/install/debug.md`).

```bash
export NRN_PROFILE_REGIONS=timestep,matrix-solver,setup-tree-matrix
```

Best for hardware counters on a **single** hot region after Layer 1 identifies it.

---

## Prioritized optimization targets

Rationale ordered by expected impact on Traub `use_gap=0` (12.9× gap), informed by
Phase C architecture and C5 profiling hypotheses.

### P1 — Host post-solve path every step (high)

`post_solve_needs_host_fallback()` returns true on EXTRACELLULAR builds (Traub uses
`secondorder=2`). Each step:

1. `sync_rhs_to_host_after_solve`
2. Host `second_order_cur` + `nrn_update_voltage`
3. Extra voltage sync before next step

CoreNEURON keeps solve + voltage update on device. **Target:** restore device post-solve
parity for non-extracellular Traub (fix `second_order_cur_on_device` / ion SOA sync),
matching C4 intent without breaking gap raster.

### P2 — Per-step recording download (`gpu_download_flush=1`) (high)

Benchmark sets `gpu_download_flush=1` → `batch_download_post_solve` every step for HOC
recording compatibility. CoreNEURON benchmark path avoids this pull pattern.

**Target:** widen batched download interval when no live recordings; or device-side
spike detection without full voltage pull (Phase B download batching already partial).

### P3 — `lastpart` / nonvint on host (high)

Without gaps, `nrn_fixed_step_lastpart` runs in `fixed_step_thread`: mechanism `state`,
`long_difus_solve`, `AFTER_SOLVE`, `fixed_record_continuous` on **host** while GPU
state may be stale.

**Target:** device `state` kernels + selective SOA sync (mirror CoreNEURON
`nrn_thread_table_check` / mechanism upload policy).

### P4 — Matrix setup sync churn (medium)

Even with C5 `matrix_rhs_d_stays_on_device_for_solve`, `nrn_rhs`/`nrn_lhs` still call
`sync_matrix_to_device_after_mechanisms` multiple times per step and zero rhs/d on host
before mechanism GPU kernels.

**Target:** single device-resident rhs/d pipeline; defer host visibility to post-solve
only when recordings require it.

### P5 — Host `deliver_net_events` (medium)

Native path delivers network events on host before GPU integrate. CoreNEURON fuses more
of the event path.

**Target:** audit event queue device residency; reduce pre-step host walks.

### P6 — Hines solver launch path (lower — likely not 12×)

Both backends use permute-2 / `solve_interleaved2` on GPU. Nsight should confirm solver
is a small fraction vs P1–P3. Optimize only if Layer 2 shows solver dominance.

### P7 — Mechanism kernel efficiency (tune after transfers)

Traub has many mod mechanisms (`naf`, `kdr`, synapses, …). After transfer overhead drops,
use `nrn_mech_wtime_` or Caliper `cur-*` to rank mechanism hotspots; consider SOA layout
and launch granularity.

**Deferred:** gap partrans (`use_gap=1`), MPI multi-rank scaling, multithread (`nthread>1`).

---

## Work packages

| ID | Summary | Exit criterion |
|----|---------|----------------|
| D0 | Branch + phase timer + this doc | Timer summary on Traub; branch pushed when approved |
| D1 | Baseline profile matrix | Layer 1+2 tables for native vs CoreNEURON; % time per phase |
| D2 | Post-solve on device for Traub | `use_gap=0` spikes match; runtime ↓ vs D0 |
| D3 | Download / recording sync policy | Correct spikes with `gpu_download_flush=0/1`; runtime ↓ |
| D4 | Device nonvint / lastpart | `state-*` on device; ringtest + Traub pass |
| D5 | Matrix setup sync reduction | Fewer `matrix-sync` calls; spikes pass |
| D6 | Traub sign-off vs CoreNEURON | Native ≤ 2× CoreNEURON GPU (stretch: ≤ 1.5×) on T1000 |
| D7 | Gap + 6-spike investigation | Optional after D6 |

---

## Commit / review policy

- Author: **Grok** (`Grok <grok@x.ai>`)
- **Do not push** until Michael approves
- Each optimization: ringtest CTest + `run_benchmark.py` `use_gap=0` before next item
- Record results in `~/models/82894/benchmark_comment_<sha>.md` at phase milestones