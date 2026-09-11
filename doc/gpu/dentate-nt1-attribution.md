# Dentate nt1 exclusive GPU: attribute native ≈3.4× vs CN GPU

**Portfolio:** GPU-native · **Phase:** `GPU-P4-dentate-nt1`  
**Tip (campaign):** `local/gpu-native` @ `76ba78245`  
**Hypothesis:** `H-dentate-1rank-cngpu` (measured; this file is how to split it)  
**Campaign (wall + identity):** `~/neuron/devbench/campaigns/2026-09-10-tip-psolve-matrix`

## Frozen cell (do not mix)

| Field | Value |
|-------|--------|
| Model | reduced dentate `max_cells=100`, `tstop=10`, `dt=0.025` (~400 steps) |
| Topology | **1 rank**, `nthread=1`, exclusive T1000 |
| Identity | ✅ 400 spikes vs CPU of the same config (`cpu_same_cell`) |
| Wall method | 3× `psolve` in one process; **warm** = last two |
| Native warm | **1.796–1.809 s** (`psolve=`) |
| CN GPU warm | **0.529–0.532 s** (`Solver Time`) ≈ **3.4×** |
| CPU warm | **2.180–2.186 s** (native is already faster than CPU) |

**Out of this cell**

- 4-rank MPS product (~1.2 s) — different topology; that is why 2026-08-10 mixed bars.
- Dentate **nt4** GPU (native ~3.0 s, CN GPU ~0.94 s) — host threads vs one GPU; slower, not the exclusive ratio.
- Recoding CURRENT/STATE density, ion SoA, net_buf, or NSB. **L2 closed that:** kernel avgs ≈ CN. The measured sub-residual is host NET_RECEIVE → full mech SoA H→D.

High performance is sacred. CoreNEURON is a **guide** (full GPU step, little host traffic), not a cage. Heap-free `weight_index` only.

## Why the old “1-rank ≈ CN” line was wrong

2026-08-01 profile compared **1-rank native ~3.3 s** to **4-rank CN Solver ~3.24 s**. Exclusive CN GPU at 1-rank was never in that table. After H4/B/E/Traub density, 1-rank native is ~1.80 s; exclusive CN GPU is ~0.53 s. The 3.4× is a real exclusive-GPU residual, not MPS.

## Prior from the closed campaign (no new run)

Native `*** computation time at t=N ms` (warm i=1 / i=2):

| t (ms) | Warm i=1 (s) | Warm i=2 (s) |
|--------|--------------|--------------|
| 0 | 0.217 | 0.213 |
| **1** | **0.736** | **0.730** |
| 2–9 | 0.105–0.117 each | 0.106–0.115 each |

CN GPU does not print that per-ms series during `psolve` (progress bar only). `--mindelay=1.0081` on the CN log; native t=1 is the first delay window.

**Implication before timers:** ~0.62 s of the ~1.27 s gap (1.80−0.53) sits in **one millisecond**. The other ~9 ms at ~0.11 s/ms ≈ **1.0 s** is still ~**2×** CN’s whole 0.53 s. Attribution must split **burst (t≈1)** vs **steady step**.

CN cell stats (same model): 397 cells, 12003 compartments, 503 presyns, 56987 synapses, 3422 point processes, 348 transfer sources/targets, 400 spikes.

## Layers (run in this order)

Timers **reset** at each `psolve` start (`ncs2nrn_integrate`) and **print** at `finalize_psolve_download`. Three psolves → three summaries. Nested buckets **double-count** `tracked-total` — report **coarse** vs psolve wall, sub-buckets as **absolute seconds**.

Coarse (sum ≈ psolve, not % of tracked-total):

`deliver-events`, `vecplay-sync`, `setup-tree-matrix`, `matrix-sync`, `matrix-solver`, `post-solve`, `download-flush`, `lastpart`, `gap-sync`

Nested (absolute s only):

| Parent | Children |
|--------|----------|
| deliver-events | thresh, tq, nrb (+ nrb-order/upload/launch/finalize/nsb) |
| setup-tree-matrix | setup-rhs, setup-lhs |
| lastpart | play, xfer, nonvint, record, deliver |
| gap-sync | gather, host, insrc, scatter |

### L1 — Phase timer + gap traffic (first explore)

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_GPU_BACKEND_TEST=native NRN_GPU_PERMUTE=2 OMP_NUM_THREADS=1
export NRN_NATIVE_GPU_PHASE_TIMER=1   # also enables NRN_GAP_TRAFFIC_STATS
export NRN_DENTATE_ENGINE=gpu NRN_DENTATE_NTHREAD=1
export NRN_TEST_TSTOP=10 NRN_TEST_MAX_CELLS=100 NRN_MULTI_PSOLVE_N=3
# exclusive GPU; do not start MPS
cd ~/neuron/nrngpu/build-gpu/test/reduced_dentate_native/neuron_gpu_native
HOC_LIBRARY_PATH=templates ./x86_64/special -notatty -python \
  ~/neuron/devbench/harness/dentate_multi_psolve.py
```

Read: warm summaries (i=1, i=2). Traffic report: `full_v_pulls`, `bulk_mech_pushes`, `h2d_scalar_calls` should stay 0 on product. Gap bytes/step are allowed (348 edges).

### L2 — ACC_TIME (only after L1 names a bucket)

`NVCOMPILER_ACC_TIME=1` on **native** (and CN GPU if the bucket is kernel-side). Instrumentation **inflates wall** — do not quote ACC_TIME wall as the product number.

Parse per-kernel **avg µs** and **copyin time(us)** for `nrn_cur_*`, `nrn_state_*`, `nrn_jacob_*`, `net_buf_receive`, thresh, gap. Rank by **kernel-sum seconds**. Copyin under a kernel is present/H→D tax (H4a-class).

CN GPU comparison uses the same env on `NRN_DENTATE_ENGINE=cn_gpu` from `reduced_dentate/coreneuron_gpu`.

### L3 — Host traffic / t=1 burst (only if L1 says deliver or gap)

- deliver-* dominates t=1 → thresh vs TQ vs NRB/NSB (already sub-bucketed; do not reopen net_buf/NSB without a new wall number).
- gap-* dominates → `print_gap_traffic_stats` bytes + scalar counts.
- lastpart-nonvint + Eigen mechs (CadepK, ccanl, na8st CONSERVE) → L2 STATE avg; product still waits the stream after Eigen STATE (SEGV otherwise).

## Child hypotheses (ledger)

Open only the one L1 points at. Recode only if that child is **≥ ~0.2 s** of the 1.27 s gap **and** has a wall hypothesis.

| ID | Claim |
|----|--------|
| H-dentate-nt1-setup | setup-tree-matrix (CURRENT + axial; lhs already folded on device) |
| H-dentate-nt1-nonvint | lastpart-nonvint / Eigen STATE math or Eigen wait |
| H-dentate-nt1-deliver | t=1 deliver burst (thresh / TQ / NRB / NSB) |
| H-dentate-nt1-gap | 1-rank gap gather/scatter (348 edges) |
| H-dentate-nt1-traffic | mid-psolve full SoA or present re-copyin |
| H-dentate-nt1-launch | many unique mechs → launch tax vs CN coarse present |

## Decision gate

| If L1/L2 shows… | Then |
|-----------------|------|
| One coarse bucket ≥ ~0.4 s and ≫ CN analogue | That is the next session; one hypothesis |
| Burst at t=1 is most of the 1.27 s, steady ~CN | Event/gap path, not density recode |
| Steady ~2× and kernels ≫ CN on named mechs | Density/launch; still one mech class |
| Traffic `full_v_pulls` or `bulk_mech_pushes` ≠ 0 | Fix traffic first (sacred: no SoA pull) |
| Split across many <0.2 s buckets | Stop; no recode this phase |

Do **not** recode CURRENT/STATE density (L2: kernels ≈ CN). Do **not** mix 4-rank MPS.

## L1 results (2026-09-10, tip `76ba78245`, exclusive 1-rank)

Harness: same `dentate_multi_psolve.py` as the parent campaign; `NRN_NATIVE_GPU_PHASE_TIMER=1`. Timer tax: warm psolve **1.97–2.02 s** vs product **1.80 s**.

Warm i=2 (psolve **1.967 s**):

| Coarse | s | Nested (absolute s) |
|--------|---|---------------------|
| lastpart | **0.762** | nonvint **0.373**, deliver **0.389**, play/xfer/record ~0 |
| setup-tree-matrix | **0.195** | rhs **0.185**, lhs **0.010** (jacob folded) |
| deliver-events (start-of-step) | **0.129** | thresh 0.014, nrb **0.009** |
| matrix-solver | 0.037 | |
| gap-gather + gap-scatter | **0.037** | gap-sync 0 (device post_solve) |
| coarse-sum | 1.14 | **uncovered vs psolve ~0.83 s** |

`deliver-tq` **0.488 s** (calls=800 = start + lastpart × 400). That is host NetCon/SelfEvent, **not** NRB. NRB 0.009 s — do not reopen net_buf.

Traffic (product-clean): `full_v_pulls=0` `bulk_mech_pushes=0` `h2d_scalar=0`. Gap ~2 kB D2H + 4 kB H2D per step (348 edges). **Not** the 3.4×.

Per-ms host progress (product and L1): **t=1 is ~0.73–0.81 s** vs ~0.11 s other ms. Aligns with first `--mindelay≈1.01` TQ wave.

## L2 results (ACC_TIME; wall inflated — native warm 3.22 s, CN GPU Solver 1.02 s)

STATE/CURRENT **kernel avgs match CN** (do not recode density):

| Kernel | Native avg | CN GPU avg | per-psolve kernel-sum |
|--------|------------|------------|------------------------|
| `nrn_state_*` | — | — | native **0.31 s** / CN **0.34 s** |
| `nrn_cur_*` | — | — | native **0.15 s** / CN **0.22 s** (CN still has `nrn_cur_ion` 0.059) |
| `nrn_state_na8st` | 119 µs | 124 µs | ~0.048 / 0.050 |
| `nrn_state_Aradi_Ca` | 78 µs | 79 µs | ~0.032 both |
| `nrn_state_CadepK` | 38 µs | 40 µs | ~0.015 / 0.016 |

**Host↔device (the residual):**

| Counter | Native (3× psolve) | CN GPU |
|---------|--------------------|--------|
| OpenACC API `acc_copyin` | **308 063** transfers, **0.73 s** (~0.24 s/psolve, ~257/step) | **828** transfers, **0.009 s** (setup) |
| `upload_soa_storage_to_device` Mechanism `FloatingPoint` | **292 620** `update device`, **0.65 s** (~0.22 s/psolve, **~244 columns/step**) | (no analogue) |
| `download_soa` mech float | 1 581 copyout (end-of-psolve / init scale) | — |

The 292 620 H→D is **`upload_present_mechanism_soa_to_device`** after **host NET_RECEIVE** (codegen: WATCH / BBCOREPOINTER path, e.g. Gfluct3). Each host apply pushes **every float column** of that type. Device `net_buf_receive` is tiny (0.009 s). This sits inside `deliver-tq` (~0.49 s) and the t=1 burst.

## Decision (this session)

| Child | State |
|-------|--------|
| H-dentate-nt1-nonvint | **closed** — STATE kernels ≈ CN |
| H-dentate-nt1-setup | **closed** — CURRENT kernels ≲ CN |
| H-dentate-nt1-gap | **closed** as 3.4× cause — 0.037 s, 0 scalar |
| H-dentate-nt1-traffic (full_v / bulk_mech counters) | **closed** those counters; **open** the host-NR SoA push |
| **H-dentate-nt1-host-nr-soa** | **measured** — next recode: elide/slim `upload_present_mechanism_soa_to_device` |
| H-dentate-nt1-deliver (TQ) | **measured** — includes host NR + SoA push; re-measure after slim |
| Uncovered ~0.83 s (gap single-step host loop / OpenACC lock) | parked until host-NR SoA is gone |

**Next session:** one hypothesis — stop full-mech SoA H→D on the host NET_RECEIVE path (live RANGE only, or skip when device CURRENT already owns those fields). Re-measure 1-rank Dentate nt1 wall. Not density. Not 4-rank MPS.
