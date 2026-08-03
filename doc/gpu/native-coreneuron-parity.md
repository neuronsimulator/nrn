# Native GPU ↔ CoreNEURON functional parity

**Portfolio:** GPU-native (feature)  
**Tree:** `~/neuron/nrngpu`  
**Living tip (2026-08-03):** `local/gpu-native` @ H4 + Session B + Session E + multi-rank MPS + **Eigen STATE v_unused refresh** (kin-native green). Exclusive wall multi-warm ~**1.15–1.17 s**; dentate 4-rank MPS psolve ~**1.5–1.6 s** (spike multiset still 390 vs 400).  
**Parked explor:** `local/gpu-p4-exclusive-residual` (slim JACOB wall-flat); `local/gpu-P4-hotpath-netreceive` (Phase C wall-flat); `local/gpu-p4-setup-rhs-density` (Session E archive, **merged to tip**)  
**Handoffs:** `GROK-GPU-NATIVE.md`, `AGENTS.md`, `~/neuron/notes/PORTFOLIO.md`  
**This file:** ordered steps you can re-open without chat memory. Update **Status** at end of each session.

**North star:** CoreNEURON fixed-step **capability surface** on native GPU (tests as acceptance), with **high performance sacred** (no green-by-full-SoA-pull).

**Non-goals:** CVode GPU; bit-identical CoreNEURON internals; other portfolio rows (IDA, NMODL-CPU, …).

---

## Session continuity (read once)

### What to trust (order)

| Rank | Source | Role |
|------|--------|------|
| 1 | **Git commits** on this tree | Code truth |
| 2 | **This file** (Status tables) | Plan + where you stopped |
| 3 | **`GROK-GPU-NATIVE.md`** | Constraints, smokes, permanent rules |
| 4 | **`PORTFOLIO.md` row GPU-native** | Cross-project Next line |
| 5 | Grok chat / `/resume` | Convenience only — not succession archive |

Experimental Grok memory is optional and **not** the archive. If it conflicts with this file, **this file wins**.

### Naming (so the session picker means something)

Immediately after `/new` or when the topic stabilizes:

```text
/rename GPU-P0-triage
```

| Pattern | Meaning |
|---------|---------|
| `GPU-P0-triage` | Phase 0 only |
| `GPU-P1-spikes` | Phase 1 cluster (spikes/direct/fornetcon) |
| `GPU-P1-events` | netmove / watch / random |
| `GPU-P1-state` | datareturn / imem / array transfer / units |
| `GPU-P1-control` | psolve / ba / pointer |
| `GPU-P2-mpi-gap` | MPI / gap / subworlds |
| `GPU-P3-models` | dentate / testcorenrn / heavier |
| `GPU-P4-perf` | Baselines, host traffic, phase timers |
| `GPU-P4-gap-scatter` | Gap bulk scatter (closed on tip) |
| `GPU-P4-density` | setup-tree-matrix + lastpart-nonvint launch density |
| `GPU-P4-multirank` | Multi-rank GPU share / MPS (ops closed on tip) |
| `GPU-hygiene` | full-ctest noise not native product |

**One living session per phase** (or cluster). Prefer **resume** that named session until the phase Status is done. When context is bloated or the agent is lost: **end checklist below → `/new` → paste the phase starting prompt** — do **not** resume a year-old auto-title.

Picker help: type `GPU-P` in `/resume` filter; or `grok sessions search GPU-P0`.

### End-of-session checklist (every phase)

1. Update **Status** tables in **this file** (red/green/notes).  
2. If code landed: **commit locally**, do **not push** unless asked.  
3. One-line **Next** at the bottom of this file (and PORTFOLIO if the portfolio Next changed).  
4. `/rename` still matches the phase (or rename to next phase before exit).  
5. Optional: note session id in the ledger below (`/session-info`).

### Permanent: when the topic is done → new session (do not skip)

If there are **no further prompts for the current named topic**, the agent should **offer a clean handoff** (not keep dilating the same auto-title). Pattern:

```text
Unless you have further prompts for this session:
1. Rename this session to <accurate closed title>   # e.g. GPU-P4-multirank
2. Start a new session named <next title>           # e.g. GPU-P4-density
3. Starting prompt: <paste block from this file or PORTFOLIO>
```

- Prefer **one living session per phase/cluster** (`GPU-P4-density`, not a year of “GPU-P4-gap-scatter”).  
- Closed work keeps a **stable archive name** in the picker; next work gets a **new** `/new` + starting prompt.  
- Same rule lives in **`AGENTS.md`** and **`~/neuron/notes/META-ORG.md`**.

### Session ledger (optional human map)

Fill as you go. UUID is from `/session-info`; title is from `/rename`.

| When | Title (`/rename`) | Session id (short) | Commit / note |
|------|-------------------|--------------------|---------------|
| 2026-07-29 | (plan) | — | Plan created; no code |
| 2026-07-29 | GPU-P0-triage | — | P0: classify A–D; harness green; CMake NONVINT for G4 native; fornetcon native green |
| 2026-07-29 | GPU-P0-triage | — | Device nonvint mandatory under native; full NONVINT env removal; fail closed (no host STATE) |
| 2026-07-29 | GPU-P1-spikes | — | ACC IClamp: CMake `stim$` match (not netstim); at_time device bind; trajectory staging once/psolve; QUALIFIED yes for direct/spikes; native V residual (device t + electrode sav) |
| 2026-07-29 | GPU-P1-spikes | — | Host-captured `_nrn_thread_t` for ACC CURRENT/STATE (IClamp window); modes 0–1 direct/spikes green; mode 2 continuerun+psolve deferred |
| 2026-07-29 | GPU-P1-events | — | Spikes cluster closed (file_mode = same residual); events triage: netmove/nmodlrandom QUALIFIED no (host DAsyn/noisychan); watchrange GPU assert |
| 2026-07-29 | GPU-P1-events | — | netmove native green: ACC DAsyn dedicated special + GPU header stage for nrnivmodl |
| 2026-07-29 | GPU-P1-events | — | nmodlrandom native green: ACC noisychan; RANDOM Instance fix; host INITIAL for RANDOM |
| 2026-07-29 | GPU-P1-events | — | watchrange native green: host WATCH/NET_RECEIVE OK; fix device→host SoA download clobber of host-authoritative mechs (Bounce) |
| 2026-07-29 | GPU-P1-state | — | units + array_variable_transfer native green: ACC UnitsTest (host INITIAL for nrn_ghk) + ACC green/red dedicated specials |
| 2026-07-29 | GPU-P1-control | — | pointer native green; psolve multi-psolve green (skip continuerun under native — mode-2 residual) |
| 2026-07-29 | GPU-P1-control-B | — | fast_imem control green: ACC present_fp local-id index (multi-thread SEGV); host ELECTRODE sav_rhs post-pass |
| 2026-07-29 | GPU-P1-control-B | — | datareturn control green (cpu+gpu): same ACC multi-thread / electrode fix; native still Exp2Syn QUALIFIED |
| 2026-07-29 | GPU-P1-state | — | fast_imem native green: ELECTRODE sav via host RMW of device i/sav (memcpy) + `_nrn_thread_t` in nrn_current (IClamp window). modes 0–1 direct imem green; mode 2 continuerun still deferred |
| 2026-07-29 | GPU-P1-state | — | datareturn native green (modes 0–1): ACC built-in Exp2Syn; download ion SoA after device CURRENT; skip mode 2 under native |
| 2026-07-29 | GPU-P1-control | — | test_ba native green: host BEFORE/AFTER mechs exempt from Gate B/C (shared RANGE with host CURRENT/STATE) |
| 2026-07-29 | GPU-P1-control-B | — | test_natrans control green: set_permute via valid_cell_permute (GPU forbids 0). Native still D (partrans partial-present) → P2 |
| 2026-07-29 | GPU-P2-mpi-gap | — | test_natrans native green (nthread=1): host nai→napre + device SoA sync; multi-thread partrans partial-present residual |
| 2026-07-29 | GPU-P2-mpi-gap | — | spikes_mpi native green (mode 0): multi-rank spike match; mode 1 re-psolve on shared GPU residual; mode 2 continuerun residual; serial modes 0–1 product (mode 2 skipped) |
| 2026-07-29 | GPU-P1-mode2 | — | Mode-2 product: GPU fixed-step psolve-scoped only (host continuerun=CPU, CoreNEURON-like). Reseed PreSyn hysteresis from host V at psolve entry. test_psolve native green; mode-2 alone spike match (±fast_imem). Sequential 0→1→2 without imem green; with fast_imem residual (OpenACC partial-present). |
| 2026-07-29 | GPU-P2-mpi-gap | — | Design: `doc/gpu/native-partrans.md` — buffer-first gap path (CoreNEURON-like); pure same-thread GPU deferred. |
| 2026-07-29 | GPU-P2-mpi-gap | — | S0+S1: native gap buffer path + ACC HalfGap; 1-rank ringtest gap 128 spikes match CPU (sorted). |
| 2026-07-29 | GPU-P2-mpi-gap | — | S2: multi-rank MPI gap green — live `v_node_index` MPI outsrc gather (`deviceptr`); 2-rank 128 spikes match sorted `spk2.gap…ref`; ctest via `h.nrnmpi_init` (not `special -mpi` OpenACC SEGV). |
| 2026-07-30 | GPU-P2-mpi-gap | — | Fail-loud native policy: threshold detect + gap gather/scatter no longer silent host fallback; opt-in `NRN_GPU_THRESH_HOST_FALLBACK` / `NRN_GPU_GAP_HOST_FALLBACK`; counters + `NRN_GPU_THRESH_STATS`. |
| 2026-07-30 | GPU-P2-mpi-gap | — | S3 partial (then closed): multi-thread gap buffers; later full green `-gap -nt 2/4` (PsolveGpuScope process-wide + OpenACC host mutex + hit list off NrnThread). |
| 2026-07-31 | GPU-P2-mpi-gap | — | MPI native/gap ctests green (`nrnmpi_init`); partial-present ownership; thresh hit list off NrnThread; ctest `neuron_gpu_native_nt2_gap`; Status hygiene (no longer SEGV D). |
| 2026-07-31 | GPU-P2-netcon-thresh | — | NetCon-heavy multi-thread threshold present residual closed: free-before-copyin in `target_copyin` + OpenACC lastpart serialize; `test_natrans` native uses NetCon topology nthread=4. |
| 2026-07-31 | GPU-P2-mode-leftovers | — | spikes_mpi mode-1 re-psolve + sequential mode-2+fast_imem green (same free-before-copyin era); product modes 0–2 for serial/MPI spikes + direct. |
| 2026-07-31 | GPU-P3-models | — | P3 start: reduced_dentate controls green; testcorenrn conc/deriv/kin **online native** green. |
| 2026-07-31 | GPU-P3-models | — | More online: bbcore native green; launcher NRN_TEST_HOC fix (false host greens); Vector.play H→D push; vecplay/watch residual. |
| 2026-07-31 | GPU-P3-models | — | vecplay native green (202 spikes): no unconditional host V H→D after fixed_play; column H→D of played RANGE only. |
| 2026-07-31 | GPU-P3-models | — | watch native green (173 spikes): nmodl WATCH host activate; host NET_RECEIVE for WATCH mechs + SoA H→D. |
| 2026-07-31 | GPU-P3-models | — | ACC codegen: present_fp for host NET_RECEIVE/HOC wrappers; net_buf_receive node_data/ppvar/dptr; vecevent ACC builds; Gfluct3 still ACC-device residual. |
| 2026-07-31 | GPU-P3-models | — | Gfluct3 native green (182 spikes): device-safe mynormrand (no static `_ran_compat` in ACC); host NET_RECEIVE for BBCOREPOINTER + SoA push; BEFORE BREAKPOINT folded into CURRENT; `testcorenrn_gf_native`. |
| 2026-07-31 | GPU-P3-models | — | vecevent native green (60 spikes sorted): ACC VecStim host NET_RECEIVE + net_event; 4-rank via `NRN_TEST_MPI`/`nrnmpi_init`; `testcorenrn_vecevent_native`. reduced_dentate residual: ACC mechs link + runs; GC EPSP missing (~10 MPP vs ~400). |
| 2026-07-31 | GPU-P3-models | — | reduced_dentate native green (400 spikes): gap deferred-lastpart left `compute_gpu=0` so post-step NET_RECEIVE never flushed to device; force device deliver/flush in `deliver_post_step_events_host`. `reduced_dentate_native` 4-rank product. |
| 2026-07-31 | GPU-P3-models | — | P3 product close: patstim **N/A** for device (host art-cell PatternStim; NMODL host path only). Online native matrix closed for CoreNEURON-GPU-like scope. Full NMODL `ctest` greening out of scope (separate session). |
| 2026-07-31 | GPU-P4-perf | — | Baselines on T1000; host-traffic audit; fix mid-psolve full SoA finalize on gap single-step path. |
| 2026-07-31 | GPU-P4-perf | — | Instrument deferred gap lastpart: `gap_sync` + `lastpart` wall around `nrn_fixed_step_deferred_gap_lastpart` (tracked ≈ full gap runtime). |
| 2026-08-01 | GPU-P4-gap-scatter | — | Bulk gap scatter (pack vals + device field-base write); de-chatty vs O(n) scalar H→D. Side branch `local/gpu-p4-gap-scatter`. |
| 2026-08-01 | GPU-P4-gap-scatter | — | Cherry-pick A+B + bulk scatter onto **`local/gpu-native`** (`ade7368d1`, `8dff0c0bf`). |
| 2026-08-01 | GPU-P4-lastpart-ab | — | Finer lastpart sub-buckets + remeasure; H1 prepare-wait elide abandoned (no clear win). |
| 2026-08-01 | GPU-P4-lastpart-ab | — | Cherry-pick lastpart sub-buckets onto **`local/gpu-native`** (`9ae97e8e5`). Gap-only P4 closed as residual. |
| 2026-08-01 | GPU-P4-setup-nonvint-density | — | setup-rhs/lhs sub-buckets; H1 defer per-mech ACC wait (CoreNEURON-like); ringtest noise-flat. |
| 2026-08-01 | GPU-P4-dentate-profile | — | Dentate tip timers: 4-rank psolve ~37 s; 1-rank ~3.3 s ≈ CN; contention is the multi-rank residual. |
| 2026-08-01 | GPU-P4-multirank | — | CUDA MPS ≈ CN; device_assign on tip (`db83f4adb`); handoff → density. |
| 2026-08-02 | GPU-P4-density-H4 | — | H4c NMODL product: TABLE temps stack/`double&`; live present; state_hh ~81 µs; residual thin-rates. |
| 2026-08-02 | GPU-P4-density-H4 | — | Phase 0 hot-path specialization plan: contracts, harness, baselines; Session A rates_*_state. |
| 2026-08-03 | GPU-P4-hotpath-rates-state | — | Session A: rates_*_state thin ABI; state_hh ~73 µs (vs 81); 688 green; residual vs hand ~18. |
| 2026-08-03 | GPU-P4-hotpath-rates-inline | — | Session A residual: force-inline STATE rates TABLE; state_hh ~19 µs ≈ CN; wall ~1.50–1.60 s; 688 green. |
| 2026-08-03 | GPU-P4-tip-merge-H4 | — | FF-merge H4a–c+A force-inline onto `local/gpu-native`; tip re-smoke: state_hh ~19 µs, multi-warm ~1.50–1.54 s, 688 green. |
| 2026-08-03 | GPU-P4-hotpath-current | — | Session B: force-inline thin CURRENT; cur_hh ~14 µs ≈ hand; wall multi-warm ~1.31–1.37 s; 688 green. |
| 2026-08-03 | GPU-P4-tip-merge-current | — | FF-merge Session B onto `local/gpu-native`; tip re-smoke: cur_hh ~15 µs, state_hh ~19 µs, multi-warm ~1.29–1.31 s, 688 green. |
| 2026-08-03 | GPU-P4-hotpath-netreceive | — | Phase C explor (separate branch): min-present net_buf; wall flat; no tip-merge. |
| 2026-08-03 | GPU-P4-exclusive-residual | — | Re-profile post H4+B: native ≈ CN exclusive; slim JACOB wall flat; no tip-merge. |
| 2026-08-03 | GPU-P4-setup-rhs | — | Session E: phase-boundary fences only (H1 + zero waits); setup-rhs ~0.24; wall ~1.18–1.28 settled. |
| 2026-08-03 | GPU-P4-tip-merge-setup-rhs | — | FF-merge Session E onto `local/gpu-native`; tip re-smoke: 688 green; setup-rhs ~0.24; wall settled ~1.15–1.17 s. |
| 2026-08-03 | GPU-P4-multirank-mps | — | Product multi-rank CUDA MPS: ensure_cuda_mps harness (ringtest MPI + dentate); docs. Eigen STATE full-present fix (CadepK crash). Dentate 4-rank MPS psolve ~1.5–1.6 s; ringtest 2-rank 688 green. Spike multiset residual 390 vs 400. |
| 2026-08-03 | GPU-P4-dentate-spikes | — | Eigen STATE: present+refresh v_unused for functors (H4c stack v left functors on stale SoA V). **kin-native green** (was empty spikes). Dentate still 390 vs 400 = all 10 GCs silent (na8st CONSERVE N=8 residual). Ringtest 688@100 + 2-rank MPI green. |
| 2026-08-03 | GPU-P4-dentate-exp2syn | — | **False lead closed:** end-of-run A/B=0 was `finalize_psolve_download` after sorted-token teardown (no-op). Fix: finalize inside token scope. Topology-matched Exp2Syn A/B/g match. **True first break:** t=0.05 **post_solve** all GC V=NaN (post_setup finite). Still 390 vs 400. |

---

## Phase map (ordered)

```text
P0  Triage full ctest noise + ringtest harness vs ctest
 │
P1  Product matrix: *_py_gpu (control) + *_py_gpu_native  [main work]
 │     clusters: spikes → events → state → control
 │
P2  Topology / MPI / gap (natrans, mpi spikes, subworlds, gap ringtest)
 │
P3  Heavy / extended: reduced_dentate, testcorenrn_* online native clones
 │
P4  Perf + multi-rank polish; device-resource owner only if forced
```

Do **not** require full 610-test green before P1. P0 only needs: classified failure buckets + ringtest signal trustworthy.

---

## Phase 0 — One triage session

**Goal:** Classify the ~50 `ctest` failures so P1 is not blocked by Python/hoc/coverage noise. Fix only **cheap false reds** that destroy signal (esp. ringtest ctest vs harness).

### Steps (in order)

1. Env:
   ```bash
   source ~/neuron/bin/nrnenv nrngpu build-gpu
   export NRN_GPU_BACKEND_TEST=native NRN_GPU_PERMUTE=2
   # Device nonvint is mandatory under native (no env; fail closed if Gate C red).
   nvidia-smi -L
   ```
2. Confirm install: `ninja install` if libs stale; ringtest **harness** green:
   ```bash
   cd ~/neuron/nrngpu/build-gpu/test/external_ringtest/neuron_gpu_native_mpi
   ./prcellstate_native_gpu.sh 32 1
   # optional long: ./prcellstate_native_gpu.sh 32 100
   ```
3. Run **ctest** ringtest native only; if red while harness green → fix **script/env/install** first:
   ```bash
   cd ~/neuron/nrngpu/build-gpu
   ctest -V -R 'external_ringtest::neuron_gpu_native'
   ```
4. Bucket **every** recent full-suite failure into A–D (table below). Capture one-line reason (log snippet, not essay).
5. Re-check CoreNEURON GPU **controls** that failed (`fast_imem_py_gpu`, `datareturn_py_gpu`, `test_natrans_py_gpu`, …): branch bug vs env.
6. Stop. Do **not** implement new native features in P0 unless a one-line env/install fix.

### Failure buckets

| Bucket | Meaning | Action |
|--------|---------|--------|
| **A** | Native product (`*_gpu_native`, native ringtest) | P1+ backlog |
| **B** | CoreNEURON GPU/CPU broken on this branch | Fix or quarantine before trusting controls |
| **C** | Non-GPU / branch noise (py3.14, hoctests, solver, coverage, …) | Hygiene session later; not P1 |
| **D** | Known later (gap, multi-rank heavy, offline bbcore, …) | P2/P3; note XFAIL reason |

### Status — P0

| Item | Status | Notes |
|------|--------|-------|
| Harness ringtest short | **GREEN** | tstop=1: dV=0; matrix noise max \|d\| ~1e-15. `rdcellstate` exits 1 on noise-only diffs (not product-red). |
| Harness ringtest 688@100 | **GREEN** | 688 spikes both sides; dV=0; max \|d\| ~1e-13 (noise). |
| ctest native ringtest vs harness | **reconciled → green** | Harness = 1-rank product gate (green). 2-rank ctests green via **`h.nrnmpi_init()`** launch (not `special -mpi` — OpenACC multi-process SEGV on this stack). See `neuron_gpu_native_mpi` / `_mpi_gap`. |
| Failure table filled | **done** | Clean full suite (no ambient `NRN_GPU_BACKEND_TEST`): **51/610 failed (92% pass)**. See table. |
| CoreNEURON control smokes | **green** | Green: fornetcon/direct/spikes/units/ba/psolve/netmove/fast_imem/datareturn/natrans `*_py_gpu`. |
| Cheap env fix landed | **superseded** | P0 temporary CMake `NONVINT=1` then default-on; **env fully removed**. |
| Device nonvint mandatory | **done** | No `NRN_NATIVE_GPU_DEVICE_NONVINT`; native path requires device STATE (Gate C). Host fallback removed; unqualified → informative error + report. Scaffolding ctest `*_device_nonvint_mpi` deleted. |

**Process note (do not regress triage signal):** never export `NRN_GPU_BACKEND_TEST=native` for a **full** `ctest` run. It pollutes CoreNEURON `*_py_cpu` / `*_py_gpu` into native qualification and creates mass false reds. Native wrappers already set env per-test.

**ctest parallelism:** tests registered with `REQUIRES gpu` get CTest `RESOURCE_LOCK gpu` (see `cmake/NeuronTestHelper.cmake`). Safe to run `ctest -j N` for CPU throughput; GPU jobs do not overlap.

### Failure triage table (P0 2026-07-29, clean suite)

| Test name | Bucket | One-line reason |
|-----------|--------|-----------------|
| test-solver | **C** | Catch2 SEGV in `SingleCellAndThread` (`test_solver.cpp:319`); “NrnThread 0 not permuted” warning. |
| unit_tests::benchmarks | **C** | Multicore `prun()` abort / SEGV (`test_multicore.cpp`). |
| unit_tests::gpu_config | **green** | Was SEGV: link OpenACC flags + stub `cvode_active_`/`nrn_nthread` (2026-07-31). |
| unit_tests::gpu_fadvance | **green** | Was SEGV: stub `nrn_thread_has_fixed_play` (2026-07-31). |
| unit_tests::gpu_net_receive | **green** | Was SEGV: define `nrn_is_artificial_`; product null-safe (2026-07-31). |
| ringtest (internal HOC) | **C** | SEGV in `nrn_rhs` / `finitialize` on `test/ringtest/ring.hoc` (not external GPU harness). |
| pytest_coreneuron::basic_tests_py3.14 | **C** | SEGV under Python 3.14 + CoreNEURON pytest path. |
| coverage_tests::cover_tests | **C** | `test_netcvode_cover` AssertionError on `CVode.netconlist` cover case. |
| hoctests::test_kschan_py | **C** | Trajectory length ACTUAL 68 vs DESIRED 393 (branch/numeric). |
| hoctests::test_optim_node_order_py | **C** | SEGV after import. |
| hoctests::test_thread_partition_py | **C** | SEGV after import. |
| parallel::partrans | **D** | SEGV in `nrnmpi_setup_transfer` (gap/transfer path). |
| parallel::nrntest_fast | **C** | SEGV in CVode/rhs_memb under MPI python. |
| coreneuron_modtests::fast_imem_py_cpu / _py_gpu | **green** | Fixed: ACC present_fp `[id]` (not double storage offset); host ELECTRODE sav_rhs after ACC cur. |
| coreneuron_modtests::datareturn_py_cpu / _py_gpu | **green** | Was SEGV; fixed with ACC multi-thread present_fp index + host electrode sav (same as fast_imem). |
| coreneuron_modtests::test_natrans_py_cpu / _py_gpu | **green** | Was AssertionError: hard-coded `set_permute(0)` invalid under CoreNEURON GPU (valid {1,2}); use `iter_permute_values()`. |
| Other `*_py_cpu` / `*_py_gpu` G4 | **green** | fornetcon, direct, spikes(+file), units, netmove, ba, psolve, pointer, watchrange, nmodlrandom*, array_transfer*, spikes_mpi* controls pass on clean suite. |
| coreneuron_modtests::fornetcon_py_gpu_native | **A→green** | Was false-red Gate C (opt-in NONVINT); **PASS** once device nonvint default-on. |
| coreneuron_modtests::test_nmodlrandom_syntax_py_gpu_native | **green** | PASS after NONVINT env. |
| coreneuron_modtests::test_subworlds_py_gpu_native | **green** | PASS after NONVINT env (P2-ish but already green). |
| coreneuron_modtests::direct/spikes(+file/mpi)_py_gpu_native | **A** | QUALIFIED no: Gate B+C host **IClamp** (needs ACC/device CURRENT). |
| coreneuron_modtests::datareturn_py_gpu_native | **green** | ACC Exp2Syn; ion SoA download after device CURRENT; modes 0–1 (mode 2 continuerun deferred). |
| coreneuron_modtests_native_units::test_units_py_gpu_native | **green** | ACC UnitsTest; host INITIAL for nrn_ghk. |
| coreneuron_modtests::test_netmove_py_gpu_native | **A** | QUALIFIED no: host **DAsyn**. |
| coreneuron_modtests::test_pointer_py_gpu_native | **green** | PASS. |
| coreneuron_modtests::test_watchrange_py_gpu_native | **green** | Host WATCH deliver; skip device→host of host-only mech SoA. |
| coreneuron_modtests::test_psolve_py_gpu_native | **partial** | Multi-psolve green; continuerun+native deferred (mode 2). |
| coreneuron_modtests::test_ba_py_gpu_native | **green** | Host BA mechs (hoc_reg_ba) exempt from Gate B/C; BA RANGE stays host-authoritative. |
| coreneuron_modtests::test_nmodlrandom_py_gpu_native | **A** | QUALIFIED no: host **noisychan**. |
| coreneuron_modtests::fast_imem_py_gpu_native | **green** | IClamp `_nrn_thread_t` in nrn_current; device electrode sav via post-cur host RMW (pull i/sav, scale, push sav). |
| coreneuron_modtests::array_variable_transfer_*_py_gpu_native | **A** | QUALIFIED no: host **green, red**. |
| coreneuron_modtests::test_natrans_py_gpu_native | **green** | S4 MechRange nthread=4; sparse LocalMechRange mailbox; **NetCon-heavy** multi-thread threshold green (2026-07-31). |
| external_ringtest::neuron_gpu_native_mpi | **green** | 2-rank via `h.nrnmpi_init()` + sorted spikes (not `special -mpi`). |
| external_ringtest::neuron_gpu_native_device_nonvint_mpi | **removed** | Scaffolding twin deleted; device nonvint is native default. |
| external_ringtest::neuron_gpu_native_mpi_gap | **green** | 2-rank gap 128 spikes; same `nrnmpi_init` launch; sorted spk vs ref. |
| external_ringtest::compare_neuron_gpu_native_mpi_gap | **green** | Depends on gap test; sorted spk2 match. |
| external_ringtest::neuron_gpu_native_nt2_gap | **green** | 1-rank `-gap -nt 2` CPU vs native exact 128 (locks old 64-vs-128 residual). |
| external_ringtest::coreneuron_*_mpi_threads* | **B/C** | SEGV in `nrn_finitialize` (cpu+gpu threads variants). |
| external_ringtest::compare_results | **C** | Spike ref empty vs 688 lines — dep of failed thread run, not native product. |
| reduced_dentate::* | **green** (controls) | neuron + crn cpu/gpu + compare green (2026-07-31). Native residual: ACC mechs. |
| external_nrntest | **C/D** | Timeout 1500s. |
| tqperf::coreneuron_python | **C** | Timeout 1000s. |

---

## Phase 1 — Product matrix (main feature work)

**Goal:** Green `coreneuron_modtests::*_py_gpu` (control) and `*_py_gpu_native` for the G4 list already in `test/CMakeLists.txt`, without regressing ringtest 688 / Traub 4474 / hot-path traffic rules.

**Registry:** see CMake “G4 native GPU modtests”. Scripts under `test/coreneuron/`, `test/pytest_coreneuron/`, `test/gjtests/`.

### Cluster order (do not skip ahead without a reason)

| Order | Cluster | Native tests (names abbreviated) | Theme |
|-------|---------|----------------------------------|--------|
| 1.1 | **spikes** | fornetcon, direct, spikes (+ file mode if easy) | Event path already deep |
| 1.2 | **events** | test_netmove, test_watchrange, test_nmodlrandom* | NSB / WATCH / random |
| 1.3 | **state** | datareturn, fast_imem, array_variable_transfer_*, test_units | Host-visible state; trajectory/sparse gather |
| 1.4 | **control** | test_psolve, test_ba, test_pointer | Lifecycle / BA / POINTER |

For each test:

1. Run CoreNEURON GPU control first (`*_py_gpu`). If red → fix **B** before native.  
2. Run native (`*_py_gpu_native`) with env above; launch is `special -python` (see CMake).  
3. Debug with progressive ladder (spikes → gid → prcellstate → phase → rdcellstate).  
4. Prefer sparse/trajectory-style host traffic over full SoA pull.  
5. Commit when one test or one coherent fix is green; update Status.

### Status — P1

*(P0 seed: control column from clean suite / serial; native after NONVINT env fix. Do not treat as P1 work complete.)*

| Test | Control `*_gpu` | Native `*_gpu_native` | Notes |
|------|-----------------|------------------------|-------|
| fornetcon | green | **green** | NONVINT env fixed Gate C false red |
| direct | green | **green** | modes **0–2** product (+fast_imem); sequential 0→1→2 green. |
| spikes | green | **green** | modes **0–2** product (+fast_imem); sequential 0→1→2 green (serial + MPI). |
| spikes_file_mode | green | **green** | Native ignores CoreNEURON file_mode; same modes 0–1 product path as `spikes_py_gpu_native`. |
| fast_imem | **green** | **green** | Host electrode sav + multi-thread ACC index; device: post-cur memcpy RMW of i→sav (avoids illegal address of in-ACC sav present); IClamp window uses host-captured t in nrn_current. |
| datareturn | **green** | **green** | ACC Exp2Syn; ion SoA download. Native modes **0–1**; mode-2 product via test_psolve (sequential multi-mode residual). |
| test_units | green | **green** | ACC UnitsTest via `coreneuron_modtests_native_units`; host INITIAL when `nrn_ghk` (not device-callable). |
| test_netmove | green | **green** | ACC `DAsyn` via dedicated special group `coreneuron_modtests_native_netmove` (nmodl `--c acc --oacc`); modes 0–2 pass. |
| test_pointer | green | **green** | Built-in ACC path + POINTER; was Status lag after IClamp ACC. |
| test_watchrange | green | **green** | Host WATCH + SelfEvent NET_RECEIVE on Bounce; full SoA download no longer clobbers host-authoritative mechs (no CURRENT/STATE device phase). |
| test_psolve | green | **green** | Host continuerun + multi-psolve: GPU psolve-scoped; hysteresis reseed. CoreNEURON-equivalent mode-2 control path. |
| test_ba | green | **green** | Host BEFORE/AFTER mechs skip Gate B/C (shared `inc` with host CURRENT/STATE); residual ACC `hoc_reg_ba` still open if BA+device CURRENT needed later. |
| test_nmodlrandom | green | **green** | ACC `noisychan` via `coreneuron_modtests_native_nmodlrandom` (RANDOM host INITIAL; CURRENT/STATE ACC). |
| test_nmodlrandom_syntax | green | **green** | after NONVINT |
| test_natrans | **green** | **green** | Control: valid permute. Native: S4 MechRange nthread=4 + **NetCon-heavy** topology (threshold multi-thread residual closed 2026-07-31). |
| array_variable_transfer_* | green | **green** | ACC green/red via `coreneuron_modtests_native_array_transfer`; modes 0–2 + file_mode pass. |
| spikes_mpi* | green | **green** | Native product modes **0–2** (multi-rank re-psolve + mode-2+fast_imem closed). Launch via `nrnmpi_init` / mpiexec special. |
| test_subworlds | green | **green** | after NONVINT; still P2 if expanded |

---

## Phase 2 — MPI / gap / topology

| Item | Status | Notes |
|------|--------|-------|
| spikes_mpi / file mode native | **green** (modes 0–2) | mode-1 re-psolve + sequential mode-2+fast_imem closed (2026-07-31) |
| test_subworlds native | **green** | |
| test_natrans native | **green** (nthread=4, S4, NetCon) | NetCon-heavy multi-thread threshold present residual **closed** (free-before-copyin + lastpart OpenACC serialize) |
| ringtest gap native + compare | **green** (1+2 rank; **nt=1,2,4** exact 128) | S0–S5. Multi-thread: process-wide PsolveGpuScope + OpenACC host-API mutex + thresh hit list off `NrnThread` (2026-07-31). ctest: `neuron_gpu_native_mpi_gap`, `compare_…`, `neuron_gpu_native_nt2_gap`. |
| ringtest non-gap native MPI | **green** | `neuron_gpu_native_mpi` 2-rank via `nrnmpi_init`. |
| multi-rank device assign | **shared OK** | 2-rank on shared T1000. **Always launch multi-rank native with `h.nrnmpi_init()`** — `special -mpi` OpenACC multi-process SEGV on this stack (do not re-chase as product red). |

---

## Phase 3 — Heavy models / extend coverage

| Item | Status | Notes |
|------|--------|-------|
| reduced_dentate neuron / crn cpu / crn gpu / **native** | **green** | Controls + `reduced_dentate_native` 4-rank (ACC mechs, `nrnmpi_init`) **400** spikes match sorted. Root cause was **gap deferred lastpart**: `finalize_nonvint` restored `compute_gpu=0` before post-step deliver → host NET_RECEIVE / no device flush (GC EPSP missing). Fix: force `compute_gpu=1` for device deliver+flush in `net_events.cpp`. |
| testcorenrn online native: conc, deriv, kin, bbcore, vecplay, watch, **gf**, **vecevent** | **green** | ACC special; `run_native_gpu.py` via **NRN_TEST_HOC**. gf 182; vecevent 60 (4-rank `NRN_TEST_MPI`/`nrnmpi_init`). BBCOREPOINTER: host NET_RECEIVE + SoA push; Gfluct3 BEFORE folded into CURRENT. |
| testcorenrn online native: patstim | **N/A** (device) | PatternStim is host ARTIFICIAL_CELL + BBCOREPOINTER; supplies spikes on CPU. Not a Gate B/C device mech. Optional thin native smoke only; NMODL host/ACC *codegen* quality is separate (not full-ctest gate for GPU-native). |
| nmodl table/kinetic GPU native if missing | **partial** | kin/deriv product via testcorenrn above; table-specific only if a CoreNEURON-GPU product test needs it. Full NMODL CPU ctest matrix → separate session. |
| Offline / saverestore | **CoreNEURON-only** (default) | Only if product needs; not native P3 gate. |

---

## Phase 4 — Perf / architecture debt

**Machine (2026-07-31):** NVIDIA T1000 8GB, driver 595.71.05. Env: `NRN_GPU_BACKEND_TEST=native`, `NRN_GPU_PERMUTE=2`. Times are wall / ringtest `runtime=` (psolve) where noted. CoreNEURON is a **guide**, not a target to match.

### Baselines (pre-fix → post-fix where changed)

| Case | Native | CoreNEURON GPU | Ratio (native/CN) | Notes |
|------|--------|----------------|-------------------|-------|
| Ringtest **no-gap** 1-rank tstop=100, 688 spikes | runtime ~2.4–3.3 s (warm ~2.5–3.1); wall ~2.9–4.0 s | runtime ~1.50–1.56 s; wall ~2.0–2.2 s | ~1.6–2.0× | Product green. Phase timer: setup-tree-matrix ~35%, lastpart ~45%, download-flush ~0. |
| Ringtest **gap** 1-rank tstop=100, 128 spikes | **pre** ~11–12 s; post-SoA ~8.3 s; **post-scatter ~2.6–3.1 s** | runtime ~1.59 s; wall ~2.1 s | ~1.7–2× after bulk scatter | Single-step `nrn_fixed_step` when `nrnthread_v_transfer_`. 0 scalar H→D/step. |
| reduced_dentate max_cells=100 tstop=10 **4-rank** | **no MPS** psolve ~**37–38 s**; **with MPS (post H4+E tip)** psolve ~**1.5–1.6 s**, wall ~4.5–5 s | wall ~6.3 s; Solver ~**3.24 s** | no-MPS ~11×; **MPS ≲ CN** | Root cause: multi-process thrash without MPS. Product harness auto-starts MPS. Spike multiset residual **390 vs 400** (run completes; compare red). |
| reduced_dentate same model **1-rank** | historical psolve ~**3.3 s** (pre-H4); re-smoke after Eigen present fix if needed | (CN usually 4-rank) | exclusive ~CN | No MPS needed when exclusive. |

Phase timer (`NRN_NATIVE_GPU_PHASE_TIMER=1`):

- **Non-gap ringtest (post SoA fix + instr):** setup-tree-matrix ~39%, lastpart ~41% (calls=4000 nested), deliver/matrix-solver ~9% each; tracked ≈ runtime (~2.6 s); no gap-sync.
- **Gap ringtest (pre SoA fix):** 4000 summaries (= every dt finalize); download-flush ~1.7 s; tracked only ~3.5 of 12 s (deferred path **uninstrumented**).
- **Gap ringtest (post SoA fix, pre instr):** 1 summary; download-flush gone; tracked ~1.6 of ~8.3 s (gap/lastpart still dark).
- **Gap ringtest (post instr, 2026-07-31):** 1 summary; **tracked ≈ runtime** (~8.45 / 8.46 s). Breakdown:
  - **gap-sync ~50%** (calls=4000) — `nrnmpi_v_transfer_` gather/MPI + `nrn_native_gap_targets_to_device`
  - **lastpart ~29%** (calls=4000) — deferred multi-thread lastpart wall (includes `thread_transfer` + STATE + deliver)
  - **setup-tree-matrix ~14%**, deliver/solver/post-solve remainder
- **Dentate (post SoA fix, pre gap instr):** 4 summaries; setup-tree-matrix ~16 s of tracked ~18 s; superseded by 2026-08-01 re-profile below.
- **Lastpart sub-buckets (on tip 2026-08-01):** nested under coarse lastpart (prefer absolute seconds). Gap/no-gap same shape: **setup-tree-matrix + lastpart-nonvint** co-dominate; host gap xfer ~0; scatter ~5%. Gap-only P4 residual **closed** as general fixed-step density (~1.7–2× CN, not gap-chat).
- **Setup sub-buckets + density H1 (`local/gpu-p4-setup-nonvint-density`):**
  - **setup-rhs / setup-lhs** nest under setup-tree-matrix (rhs ≈ CURRENT+axial larger than lhs/JACOB on ringtest).
  - **H1:** ACC codegen no longer waits after every CURRENT/STATE/JACOB (CoreNEURON-like); fences remain at axial, net_send flush, electrode sav, INITIAL, net_buf_receive, finalize_nonvint. Product green (checkpoint dV=0, 688/128 spikes). **Ringtest multi-warm: no clear win vs noise** (~2.6–3.5 s gap, ~2.5–2.8 s no-gap) — keep on explor; re-measure on dentate/heavier models before tip.
  - Prior abandoned: prepare_nonvint `_t` wait elide.
- **Density H2 (`local/gpu-P4-density-H2`):** slim present via `deviceptr` for SoA floats + matrix — product green; multi-warm **flat** vs tip at nring=16 and **nring=160**. Do not tip-merge.
- **Density H3 (`local/gpu-P4-density-H3`, docs):** nsys+ACC_TIME — `nrn_state_hh` ~77% GPU / ~6× CN; launch count ≈ CN; present/copyin tax. **H3b:** native TABLE `state_hh` avg ~**796 µs** vs analytic ~**177 µs** (~4.5×); CN TABLE ~19 µs (healthy). `usetable_hh` via `NRN_HH_USETABLE`.
- **Density H4a (`local/gpu-P4-density-H4`, 2026-08-02) — TABLE global H→D fix:**
  - **Root cause:** ACC codegen did `update device (hh_global)` on **every** CURRENT/STATE/JACOB launch. `hh_global` holds full TABLE arrays (6×201 doubles) plus scalars → per-step full H→D of tables (and other mechs with globals).
  - **Fix:** dirty flag `*_device_stale`; H→D only when stale; set stale after host TABLE rebuild (`print_after_host_table_rebuild`); first `copyin` clears stale.
  - **Product:** phases=1, dV=0, **688** spikes (TABLE and analytic).
  - **ACC_TIME `nrn_state_hh` avg (tstop=100, nring=16):**

    | mode | H3b (pre) | H4a (post) |
    |------|-----------|------------|
    | TABLE (`usetable=1`) | ~**796 µs** (kern sum ~3.19 s) | ~**150 µs** (kern sum ~0.60 s) |
    | analytic (`usetable=0`) | ~**177 µs** | ~**176 µs** |

  - TABLE is no longer sick vs analytic (now **slightly faster**, CN-like). Still ~**8×** CN TABLE (~150 vs ~19 µs) — residual is shared STATE quality / present tax (**H4b+**).
  - **Wall multi-warm:** TABLE ~2.04–2.48 s; analytic ~2.06–2.10 s (noise; modest vs H3b TABLE 2.43–2.65). Do **not** tip-merge until wall win is clearer or bundled with H4b; keep on explor.
- **Density H4b (`local/gpu-P4-density-H4`, 2026-08-02) — rest of state_hh vs CN:**
  - **Inventory:** STATE math uses **V + own gates/rates SoA only**. `ena`/`ek` ion loads in STATE are **dead boilerplate** (same as CN); not the native−CN gap.
  - **H4b.0 baseline (post-H4a):** TABLE `state_hh` avg ~**131 µs**; analytic ~**172 µs**; product phases=1 green. Copyin under state_hh still ~95 ms/run (separate from kernel elapsed).
  - **H4b-D:** skip ion READ in STATE when shadow var unused on STATE path (block + procedures). HH STATE no longer loads ena/ek. Product green. **ACC_TIME flat** (~139 µs TABLE — noise vs 131). Absolute cleanup only.
  - **H4b-B blocked:** pack RANGE bases as `double* const*` for fewer helper args — nvc++ cannot track array-of-pointers in ACC present/device (`Could not find allocated-variable index for symbol _present_fp`). Named `_present_fp_N` required for OpenACC.
  - **Residual (pre hand-edit):** native TABLE ~**130–150 µs** vs CN ~**19 µs** (~7–8×).
- **Density H4c hand-edit (`build-gpu/src/nrnoc/hh.cpp` only, 2026-08-02) — rates temps on stack + slim STATE:**
  - **Not product codegen** — manual edit of generated `hh.cpp`; nmodl regen wipes. Excerpt: `doc/gpu/h4c-handedit-nrn_state_hh.excerpt.cpp`.
  - **Change (combined):** (1) minf/hinf/ninf/mtau/htau/ntau as **stack locals** (no SoA store); (2) STATE **present only m,h,n** (3 columns); (3) TABLE/analytic rates **inlined** into STATE (no 25-arg `rates_hh`); (4) `hh_global` / V via deviceptr.
  - **Product:** phases=1, dV=0, **688** (TABLE).
  - **ACC_TIME `nrn_state_hh` avg:**

    | mode | H4b baseline | H4c hand-edit | CN (H3b) |
    |------|--------------|---------------|----------|
    | TABLE | ~**131 µs** | ~**18 µs** | ~**19 µs** |
    | analytic | ~**172 µs** | ~**40 µs** | ~**43 µs** |

  - **≈ CN order of magnitude** on `state_hh`. Copyin under state_hh **gone** (slim present).
  - **Wall multi-warm TABLE:** ~**1.68–1.75 s** (was ~2.1–2.5); first clear exclusive wall move toward CN.
  - **Interpretation:** residual was **STATE kernel surface** (fat present + SoA rates temps + fat rates call), not TABLE math or ions. Product path: teach NMODL ACC to emit this shape (temps local; present live fields only; thin/inlined rates).
- **Density H4c hand-edit CURRENT (same `hh.cpp`, 2026-08-02):**
  - **Live set for CURRENT (not 25−6=19):** params gnabar/gkbar/gl/el, STATE m/h/n, g_unused → **8 SoA columns**; ena/ek/ina/ik via **ion deviceptr** + stack temps; skip minf…ntau and Dm/Dh/Dn.
  - Inlined numerical di/dv (no 25-arg `nrn_current_hh`). Excerpt: `doc/gpu/h4c-handedit-nrn_cur_hh.excerpt.cpp`.
  - **ACC_TIME avg (with STATE hand-edit also on):**

    | kernel | state-only hand-edit | + CURRENT hand-edit |
    |--------|----------------------|---------------------|
    | `nrn_cur_hh` | ~**18 µs** | ~**13 µs** (~28% better) |
    | `nrn_state_hh` | ~18–20 µs | ~17 µs (unchanged) |
    | `nrn_jacob_hh` | ~12 µs | ~9 µs |

  - **Wall multi-warm TABLE:** ~**1.51–1.54 s** (state-only hand-edit was ~1.68–1.75; pre-H4c ~2.1–2.5). Product phases=1 green.
  - **Takeaway:** CURRENT also benefits from min-present, but **STATE had the large factor**; CURRENT was already near a floor (~18 µs) so absolute win is smaller. Live-set size for CURRENT is **larger than STATE** (params + states + g + ions), as expected.
- **Dentate re-profile (2026-08-01 tip, max_cells=100, tstop=10, 400 spikes, T1000):**
  - **4-rank native:** psolve ~**37 s**, wall ~40 s; load_balance ~0.998; **0 scalar H→D/step**; gap-scatter ~0.2 s/rank (tiny). Nested tracked ~60 s (double-count). **Non-nested** per rank (≈psolve decomposition):
    - **setup-tree-matrix ~15.6 s** (rhs ~9.2 + lhs ~6.4)
    - **lastpart ~12.9 s** (nonvint ~8.4 + **deliver ~4.5** — spike-heavy vs ringtest)
    - deliver-events ~1.7 s; gap all ~0.7 s; **matrix-solver ~0.12 s** (negligible)
  - **1-rank native:** psolve ~**3.3 s** (400 spikes) — **~11× faster than 4-rank** on same GPU; shape still setup + lastpart-nonvint (+ deliver).
  - **CN-GPU 4-rank:** Solver Time ~**3.24 s**, wall ~6.3 s, 400 spikes (`1 GPU shared by 4 ranks`).
  - **Conclusion:** dentate ~11× vs CN is **multi-process GPU contention**, not gap chat and not a different algorithm residual than ringtest once exclusive. 1-rank native ≈ CN solver time.
- **Multi-rank share diagnosis (2026-08-01, `local/gpu-p4-multirank-share`):**
  - **Why CN wins with 4 ranks:** CN multi-process kernels coexist on one GPU without thrashing (few host waits / larger work). Native fixed-step has denser OpenACC launch/wait traffic; **without CUDA MPS**, multi-process context switching is superlinear: 1→2 ranks 3.3→23 s, 4 ranks ~37 s.
  - **CUDA MPS:** `nvidia-cuda-mps-control -d` then 4-rank native psolve ~**2.4–3.0 s** (400 spikes), ≈ CN Solver Time / wall. **MPS is the product multi-rank policy on one GPU for native today.**
  - **Message fix:** `device_assign` never had `NRNMPI` defined on `neuron_gpu` lib → always printed “shared by 1 ranks” ×N. Fixed: define `NRNMPI=1` + launcher env local rank (`OMPI_COMM_WORLD_LOCAL_*`); rank 0 prints `shared by N ranks`. No collective `MPI_Comm_split_type` (hangs if ranks hit `gpu.enable` out of sync).
  - **Warn** when `local_size > n_gpu` and MPS control socket missing.

### Status — P4

| Item | Status | Notes |
|------|--------|-------|
| Wall-time vs CoreNEURON (guide) | **baselined (updated 2026-08-03)** | Ringtest exclusive **≲ CN** after H4+B+E: tip multi-warm settled ~**1.15–1.17 s** (Session E). CN Solver ~**1.30–1.35 s**. Pre-E tip ~1.29–1.41. Dentate exclusive ~CN; multi-rank needs MPS. Old ~1.7–2× is pre-H4. |
| Residual hot-path host traffic audit | **done** | Mid-psolve full SoA on gap path fixed. |
| Gap scatter de-chatty + A+B timers | **done (on tip)** | 0 scalar H→D/step; scatter ~5%. |
| Gap-only P4 residual | **closed as density** | Same bottleneck as no-gap (setup + nonvint); not gap transfer. |
| Lastpart sub-buckets | **done (on tip)** | play/xfer/nonvint/record/deliver. |
| Setup-rhs/lhs sub-buckets | **done (on tip)** | Nested under setup-tree-matrix; prefer absolute seconds. |
| Defer per-mech ACC wait (H1 pre-H4) | **superseded by Session E** | Pre-H4 ringtest flat; remeasured post-H4+B in Session E. |
| Slim present / deviceptr (H2) | **explor; flat nring=16/160** | `local/gpu-P4-density-H2`; no tip-merge. |
| Profile launch vs device (H3) | **done (docs)** | state_hh 77% GPU / ~6× CN; copyin tax. `local/gpu-P4-density-H3`. |
| HH TABLE vs analytic (H3b) | **done (docs)** | TABLE was ~4.5× analytic; CN TABLE healthy. |
| HH TABLE stale-global H→D (H4a) | **done (on tip)** | Dirty-flag globals; TABLE `state_hh` ~796→~150 µs. Part of H4 packet. |
| STATE dead ion reads (H4b-D) | **done (on tip)** | Skip unused ion READ on STATE path; product green; **no ACC_TIME win** alone. |
| STATE few-arg pack (H4b-B) | **blocked (OpenACC)** | `double* const*` / `_present_fp[i]` not ACC-trackable; need named bases. |
| STATE stack rates + slim present (H4c hand) | **archive (hand)** | Hand-edit reference: `state_hh` TABLE ~18 µs. Productized via H4c+A. |
| CURRENT min-present (H4c hand) | **archive (hand)** | Hand residual for Session B: `nrn_cur` ~18→~13 µs. **Met** by Session B product ~14 µs. |
| H4c NMODL productize (codegen) | **done (on tip)** | Live present + TABLE stack temps; intermediate ~81 µs before Session A. |
| Hot-path specialization plan Phase 0 | **done (docs)** | Contracts + harness + baselines below. |
| Hot-path rates_*_state (Session A Ph 1–2) | **done (on tip)** | Thin `rates_*_hh_state` emitted; STATE uses force-inline (below). Intermediate ~73 µs. |
| Hot-path force-inline STATE rates (A residual) | **done (on tip)** | Force-inline unique/safe STATE rates TABLE body. Tip re-smoke 2026-08-03: `state_hh` ~**19 µs**; wall multi-warm ~**1.50–1.54 s**; 688 phases=1 green; copy tax under state_hh **gone**. |
| H4 density packet tip-merge | **done (on tip)** | FF `local/gpu-P4-density-H4` → `local/gpu-native` (`34c8b4912`); tip remeasure matches explor. |
| Hot-path CURRENT force-inline (Session B) | **done (on tip)** | FF `local/gpu-P4-hotpath-current` → `local/gpu-native` (`f06ed70ec`). Tip re-smoke: `cur_hh` ~**14–15 µs**; wall multi-warm ~**1.29–1.37 s**; state_hh ~19 µs; 688 green; cur copy tax **gone**. |
| Hot-path NET_RECEIVE min-present (Phase C) | **explor; wall flat (parked)** | `local/gpu-P4-hotpath-netreceive`. Kernel win; ringtest wall flat. **Do not tip-merge**. |
| Post-H4+B exclusive re-profile | **done (docs)** | Native ≈ CN; setup-tree-matrix ~0.54 s dominates on tip. |
| Slim JACOB (Session D) | **explor; wall flat** | `local/gpu-p4-exclusive-residual`. Product 688 green; wall flat. **No tip-merge**. |
| Setup-stream density (Session E) | **done (on tip)** | FF `local/gpu-p4-setup-rhs-density` → `local/gpu-native` (`1dba647bb`). Tip re-smoke: product 688 green; setup-rhs ~**0.24**; setup-tree ~**0.46–0.48**; wall multi-warm settled ~**1.15–1.17 s**. |
| Dentate multi-rank profile | **done** | 4-rank thrash without MPS; **MPS ≲ CN** (tip ~1.5–1.6 s psolve). |
| Multi-rank GPU share (MPS) | **done (on tip)** | device_assign + warn; **product harness** `test/external/ensure_cuda_mps.sh` wired into ringtest MPI + dentate native ctests; docs (`native-gpu-build.rst`, `gpu-testing.rst`, AGENTS). |
| Eigen STATE min-present illegal address | **fixed (on tip)** | H4 live-present under-counted Eigen Newton functor RANGE (e.g. CadepK missing ion shadows). Full present when `eigen_newton`/`eigen_linear`. Unblocked dentate crash. |
| Eigen STATE v_unused for functors | **fixed (on tip)** | H4c stack `v` left Eigen functors reading stale `v_unused` SoA (and often un-present). STATE now presents v_unused + writes `_present_fp_N[id] = v` when Eigen solvers exist. **testcorenrn_kin_native** green. |
| Dentate spike multiset 400 | **residual (re-localized)** | **390 vs 400** = all 10 GCs silent. **False lead closed:** Exp2Syn A/B/g match CPU once `finalize_psolve_download` runs *before* sorted-token teardown (was no-op → host A/B=0). **True first break:** step 2 `t=0.05` **post_solve** — post_setup V finite (~−75), post_solve **all cell V = NaN** (CPU finite). Step 1 `t=0.025` all phases finite. Harness: `prcellstate_gc_native.sh` phases=1. Next: device post_solve / voltage update NaN on GC (not MPP deliver). |
| Single device-resource owner | open | Only if exit/leak forces. |

### H4c NMODL productize (2026-08-02, `local/gpu-P4-density-H4`)

**Codegen (`codegen_neuron_acc_visitor`):**

1. **TABLE statement vars as `double&` (`_kl_*`)** in ACC procedures; STATE loop declares stack locals (`minf`/`mtau`/…) and passes them into `rates_*` (no hot-path SoA for pure rates temps). HOC/table-rebuild bind `_present_fp_i[id]`.
2. **Per-kernel live `present`** of named `_present_fp_N` columns (AST usage). STATE HH → **m,h,n only** + `hh_global`; CURRENT → params/states/g/ion surface (skips rates temps); jacob → `g_unused` only.
3. STATE uses stack `v` (not `v_unused` SoA). OpenACC still needs **named** bases (H4b-B pack still blocked).
4. Functors keep full `present_fp` member set (deriv body ≠ procedure live set). General procedure ABI still passes non-table `_present_fp_*` (safe for VERBATIM mechs).

**Clarifications (session discussion):**

- `present` for `nrn_state_hh` is **already** 3 columns; residual is **not** fat present.
- `rates_hh` is a **device function inside** the STATE parallel loop (not a second kernel).
- MOD only has `rates(v)`; fat args are NMODL ACC inventions. Thin specialized ABI or inline is valid.
- CoreNEURON GPU does **not** min-cut columns; it uses coarse `present(inst, data, …)` + `ml->data`/Instance deviceptrs — different data model.

**Measure (nring=16, tstop=100, TABLE, product phases=1):**

| metric | H4b baseline | H4c product codegen | H4c hand-edit | CN (H3b) |
|--------|--------------|---------------------|---------------|----------|
| `state_hh` avg | ~131 µs | ~**81 µs** | ~18 µs | ~19 µs |
| `nrn_cur_hh` avg | ~18 µs | ~**17 µs** | ~13 µs | — |
| wall multi-warm | ~2.1–2.5 s | ~**1.88–1.92 s** | ~1.52 s (both edits) | — |
| product 688 dV=0 | green | **green** | green | — |

**Residual vs hand-edit:** general procedure ABI still passes unused non-table `_present_fp_*` into STATE call of `rates_hh`. Hand-edit inlined rates. Next: specialized hot-path versions (plan below).

### Session A — rates_*_state (2026-08-03, `local/gpu-P4-density-H4`)

**Codegen (`codegen_neuron_acc_visitor`):**

1. **Analysis / safety:** specialize only **PROCEDURE**s called from STATE when safe — no VERBATIM, no net_send/move/event, no ion/POINTER/RANDOM, no nested MOD procedure calls (InlineVisitor folds pure FUNCTIONs like `vtrap`). FUNCTIONs (e.g. `vtrap`) stay general only.
2. **Emit** `f_<proc>_<suffix>_state` + `<proc>_<suffix>_state` (e.g. `rates_hh_state`) with thin ABI: `inst` + live TABLE `double&` temps + live present_fp columns only + MOD args (`v`). No `node_data` / `id` / `_ppvar` / `_thread` / fat present_fp.
3. **STATE call site (thin ABI era):** used thin version when specialized; general `rates_hh` remains for HOC / table rebuild / INITIAL.
4. **STATE present_fp decls:** live columns only when all STATE MOD calls are specialized and no Eigen Newton/linear (functors need full set). Else full decls.
5. **Unit tests:** `testcodegen "[codegen][neuron][acc]"` — hh emits `rates_hh_state` thin sig.

**Measure thin ABI (nring=16, tstop=100, TABLE, product phases=1):**

| metric | H4c product (Ph0) | Session A rates_*_state | H4c hand-edit | CN (H3b) |
|--------|-------------------|-------------------------|---------------|----------|
| `state_hh` avg | ~**81 µs** | ~**73 µs** (ACC_TIME noisy; one run ~160) | ~18 µs | ~19 µs |
| `nrn_cur_hh` avg | ~17 µs | ~**20 µs** | ~13 µs | — |
| wall multi-warm | ~1.88–1.92 s | ~**2.35–2.52 s** (noise; not a clear wall win) | ~1.52 s | — |
| product 688 dV=0 | green | **green** | green | — |

Thin ABI residual: still a device **call** + residual copyin/copyout under state_hh. Milestone ≲25–30 µs not met until force-inline (below).

### Session A residual — force-inline STATE rates (2026-08-03, `local/gpu-P4-density-H4`)

**Codegen:**

1. **Force-inline** unique/safe specialized PROCEDURE body at STATE call site (TABLE path when applicable). No device call to `rates_*_state` from STATE.
2. Inlined body uses **`hh_global.*` / bare `celsius`** (present shape), not `inst.global` / `*(inst.celsius)` — hand-edit shape; kills residual copy tax.
3. **Drop unused `_thread` / `_ppvar`** from specialized STATE present and loop prolog.
4. Thin `rates_*_state` / `f_*_state` still **emitted** (available); general `rates_*` kept for HOC/table rebuild. STATE does not call them.
5. **Unit tests:** STATE must not contain `rates_hh_state(inst,`; must contain `hh_global.usetable` / `hh_global.t_minf`.

**STATE shape (product codegen now):** present only m,h,n + `hh_global` (no `_thread`); rates TABLE body inlined into STATE loop.

**Measure (nring=16, tstop=100, TABLE, product phases=1):**

| metric | Session A thin ABI | **A residual force-inline** | H4c hand-edit | CN (H3b) |
|--------|--------------------|-----------------------------|---------------|----------|
| `state_hh` avg | ~**73 µs** | ~**19 µs** | ~18 µs | ~19 µs |
| `nrn_cur_hh` avg | ~**20 µs** | ~**18 µs** | ~13 µs | — |
| wall multi-warm | ~2.35–2.52 s | ~**1.50–1.60 s** | ~1.52 s (state+cur) | — |
| product 688 dV=0 | green | **green** | green | — |
| state_hh copy tax | residual copyin/out | **gone** (`time(us): 0` under state_hh) | gone | — |

**Milestone A met:** `state_hh` ≲25–30 µs and ≈ hand/CN ~18–19. **Clear wall win** vs H4c product (~1.88–1.92 s) and Session A thin.

### Tip-merge H4 packet (2026-08-03, on `local/gpu-native`)

**Action:** Fast-forward merge `local/gpu-P4-density-H4` → living tip `local/gpu-native` (H4a dirty globals + H4b ion skip + H4c product + Session A thin + force-inline). Merge-base was tip; 8 commits, no tip-only delta.

**Tip re-smoke (nring=16, tstop=100, TABLE, phases=1):**

| metric | explor (pre-merge) | **tip after merge** |
|--------|--------------------|---------------------|
| `state_hh` avg | ~19 µs | ~**19 µs** |
| `nrn_cur_hh` avg | ~18 µs | ~**19 µs** |
| state_hh copy tax | gone (`time(us): 0`) | **gone** |
| wall multi-warm (5×, no ACC_TIME) | ~1.50–1.60 s | ~**1.50–1.54 s** (first ~1.73) |
| product 688 dV=0 | green | **green** |

Packet is product on tip. Session B CURRENT closed on explor (below).

### Session B — CURRENT force-inline / thin `nrn_current` (2026-08-03, `local/gpu-P4-hotpath-current`)

**Codegen (`codegen_neuron_acc_visitor`):**

1. **Safety gate** `current_force_inline_safe()`: no VERBATIM, net_send/move/event, POINTER/RANDOM in BREAKPOINT (+ BA). Ions allowed (unlike STATE).
2. **Force-inline** BREAKPOINT body twice for numerical di/dv (`v+0.001`, `v`) — no device call to `nrn_current_*` when safe. Skip emitting helper.
3. **Stack temps:** ion READ shadows (ena/ek) from dptrs; ASSIGNED *written* in BREAKPOINT (gna/gk/ina/ik/il). Pure RANGE reads (e.g. gap `vgap`) stay SoA present.
4. **Min present:** PARAMETER + STATE + `g_unused` only (HH: 8 columns). No `v_unused`, no intermediate SoA, no `vec_d` on CURRENT (jacob owns `vec_d`).
5. **Thin `nrn_current` fallback** when unsafe: live present_fp only + id + host t + v (no fat `_lmc`/`inst`/`node_data`). Globals via present `*_global` names.
6. **Unit tests:** Session B THEN on hh.mod — no `nrn_current_hh`, has `_cur_v` / stack `ena`/`gna`.

**Measure (nring=16, tstop=100, TABLE, product phases=1):**

| metric | tip (H4 + A) | **Session B explor** | H4c hand-edit | CN (H3b) |
|--------|--------------|----------------------|---------------|----------|
| `state_hh` avg | ~**19 µs** | ~**19 µs** | ~18 µs | ~19 µs |
| `nrn_cur_hh` avg | ~**19 µs** | ~**14 µs** | ~13 µs | — |
| wall multi-warm (5×) | ~1.50–1.54 s | ~**1.31–1.37 s** (first ~1.86) | ~1.52 s (state+cur) | — |
| product 688 dV=0 | green | **green** | green | — |
| cur_hh copy tax | residual | **gone** (`time(us): 0`) | gone | — |

**Milestone B met:** `nrn_cur_hh` ≈ hand ~13–14 µs. **Clear wall win** vs tip (~1.50–1.54 → ~1.31–1.37). Tip-merge justified; performed 2026-08-03 (below).

### Tip-merge Session B CURRENT (2026-08-03, on `local/gpu-native`)

**Action:** Fast-forward merge `local/gpu-P4-hotpath-current` → living tip `local/gpu-native`. Merge-base was tip; 1 commit (`f06ed70ec`), no tip-only delta.

**Tip re-smoke (nring=16, tstop=100, TABLE, phases=1):**

| metric | explor (pre-merge) | **tip after merge** |
|--------|--------------------|---------------------|
| `state_hh` avg | ~19 µs | ~**19 µs** |
| `nrn_cur_hh` avg | ~14 µs | ~**15 µs** (noise) |
| cur_hh / state_hh copy tax | gone (`time(us): 0`) | **gone** |
| wall multi-warm (5×, no ACC_TIME) | ~1.31–1.37 s | ~**1.29–1.31 s** (first ~1.62) |
| product 688 dV=0 | green | **green** |

Session B CURRENT is product on tip. Exclusive ringtest ≈ CN post H4+B; residual setup-rhs (Session E).

### Session E — setup-rhs launch density (2026-08-03, `local/gpu-p4-setup-rhs-density`)

**Hypothesis:** After H4+B, CURRENT/STATE kernels are thin (~14–19 µs) so **host waits after every small setup kernel** dominate setup-rhs (~0.29 s of ~0.54 s setup-tree). CoreNEURON keeps async on `nt->stream_id` through setup; fence only at phase boundaries.

**Change (one density packet):**

1. **Codegen (H1 remeasure post-H4+B):** no `wait(stream)` after CURRENT/STATE/JACOB data blocks; keep waits at INITIAL, net_buf_receive, net_send flush, electrode sav post-pass.
2. **Runtime:** no wait after `zero_matrix_rhs` / `zero_matrix_diagonal` / `transform_sav_*` — same-stream CURRENT/JACOB follow; **axial** still waits at end of rhs and lhs.

**Measure (nring=16, tstop=100, TABLE, T1000 exclusive 1-rank):**

| metric | tip H4+B | Session E explor |
|--------|----------|------------------|
| product 688 dV=0 phases=1 | green | **green** (noise-only rdcellstate) |
| setup-tree-matrix | ~**0.54** s | ~**0.46–0.47** s |
| setup-rhs | ~**0.29** s | ~**0.24** s |
| setup-lhs | ~**0.25** s | ~**0.22–0.23** s |
| wall multi-warm settled (drop cold) | ~**1.29–1.37** s | ~**1.18–1.28** s |
| CN Solver (guide) | ~1.30–1.35 s | same |

**Interpretation:** Clear **setup-bucket win** (~15% setup-tree). Modest **wall** improvement when fully warm (can undercut tip lower band and CN Solver). First runs noisier (1.5–1.8). Product green.

### Tip-merge Session E setup-stream density (2026-08-03, on `local/gpu-native`)

**Action:** Fast-forward merge `local/gpu-p4-setup-rhs-density` → living tip `local/gpu-native`. Merge-base was tip; 1 commit (`1dba647bb`), no tip-only delta.

**Tip re-smoke (nring=16, tstop=100, TABLE, T1000 exclusive 1-rank):**

| metric | explor (pre-merge) | **tip after merge** |
|--------|--------------------|---------------------|
| product 688 dV=0 phases=1 | green | **green** (noise-only rdcellstate) |
| setup-tree-matrix (phase timer warm) | ~0.46–0.47 s | ~**0.46–0.48** s |
| setup-rhs | ~0.24 s | ~**0.24** s |
| setup-lhs | ~0.22–0.23 s | ~**0.22–0.23** s |
| wall multi-warm settled (6× no ACC_TIME; drop cold) | ~1.18–1.28 s | ~**1.15–1.17** s |
| CN Solver (guide) | ~1.30–1.35 s | same |

Session E is product on tip. Phase C + slim JACOB stay parked.

---

### Hot-path ACC specialization (plan; Phase 0 closed 2026-08-02)

**Performance goals (primary):** device **`nrn_state`**, **`nrn_cur`**, **`NET_RECEIVE` / `net_buf_receive` (POINT_PROCESS)**.  
**Secondary / may stay general:** INITIAL, HOC wrappers, TABLE rebuild, rare paths.

**Design stance**

1. Kernel **`present`** = per-mech, per-kernel live SoA columns (named bases; OpenACC cannot track `double**` packs — H4b-B).
2. MOD `PROCEDURE rates(v)` only; NMODL may emit **multiple C++ versions** for distinct arg usages (hot STATE vs HOC vs table rebuild).
3. Prefer **thin specialized ABI** (args = only what that body needs at that call site). **Inline** if call is unique/small; thin call is enough if args are minimal.
4. **Safety gates** → keep **general** version only: VERBATIM, failed analysis, pointer/RANDOM device limits, etc.
5. CoreNEURON is a **perf guide** (coarse residency + thin `inst*` helpers), not a column-min algorithm to copy.

**Non-goals:** tip-merge without wall win; Traub `use_gap=1`; host NET_RECEIVE body; full SoA pull; bit-identical CN layout.

#### Acceptance harness

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_GPU_BACKEND_TEST=native NRN_GPU_PERMUTE=2
# Product gate
cd ~/neuron/nrngpu/build-gpu/test/external_ringtest/neuron_gpu_native_mpi
./prcellstate_native_gpu.sh 32 100 1   # expect 688 both sides; dV=0 (noise OK)
# ACC_TIME (nring default 16, tstop 100)
export NVCOMPILER_ACC_TIME=1
./x86_64/special -python ringtest.py -gpu-native -tstop 100 2>acc.txt
# parse avg for nrn_state_hh / nrn_cur_hh in acc.txt
# Wall multi-warm (unset ACC_TIME): 3–5× ringtest.py -gpu-native -tstop 100 | grep runtime
```

Excerpts for hand-edit reference: `doc/gpu/h4c-handedit-nrn_state_hh.excerpt.cpp`, `doc/gpu/h4c-handedit-nrn_cur_hh.excerpt.cpp`.

#### Phase 0 baselines (frozen for Session A+)

| ID | `state_hh` avg | `nrn_cur_hh` avg | wall multi-warm | product 688 |
|----|----------------|------------------|-----------------|-------------|
| H4b | ~131 µs | ~18 µs | ~2.1–2.5 s | green |
| **H4c product (Phase 0 baseline)** | ~**81 µs** | ~**17 µs** | ~**1.88–1.92 s** | **green** |
| Session A rates_*_state (thin) | ~**73 µs** | ~**20 µs** | ~**2.35–2.52 s** | **green** |
| **A residual force-inline** | ~**19 µs** | ~**18 µs** | ~**1.50–1.60 s** | **green** |
| **Session B CURRENT force-inline** | ~**19 µs** | ~**14 µs** | ~**1.31–1.37 s** | **green** |
| H4c hand-edit | ~18 µs | ~13 µs | ~1.52 s (state+cur) | green |
| CN (H3b TABLE) | ~19 µs | — | — | — |

Milestone A (rates STATE specialization): `state_hh` **≲ 25–30 µs** then ~18 — **met** (~19 µs force-inline). Product always 688 phases=1.  
Milestone B (CURRENT specialization): `nrn_cur_hh` ≈ hand ~13 — **met** (~14 µs); wall win measured.

#### Implementation phases (after Phase 0)

| Phase | Work | Exit |
|-------|------|------|
| **1** | Live-set / specialization keys + safety gates (analysis infra) | **done** unit tests |
| **2** | Multiple procedure versions; wire **`rates_*_state`** + force-inline unique STATE body | **done** ~19 µs; 688 green; wall win |
| **3** | CURRENT helpers / thin `nrn_current` (Session B) | **done** cur ~14 µs; wall ~1.31–1.37; 688 green |
| **4** | Fallback matrix + regression (VERBATIM must stay general) | suite notes |
| **5** | Tip-merge H4a+b+c+A (wall win measured) | **done** on tip 2026-08-03 |
| **6** | Tip-merge Session B CURRENT (wall win measured) | **done** on tip 2026-08-03 |

**Session order:** A–B–E closed on tip. C parked explor. D slim JACOB parked.

### Residual perf / product debt (next when reopened)

1. **Dentate GC spikes (390 vs 400):** all 10 GCs silent; other populations match. Focus **na8st** sparse+CONSERVE N=8 on native (Eigen Newton on device). Kin path greened via v_unused refresh; GC residual remains.
2. Phase C follow-up only if denser spike traffic shows wall (parked explor).
3. Optional: slim JACOB hygiene tip-merge; lastpart-deliver; **phases=0** prcellstate download needs ACC wait after Session E (phases=1 / 688@100 green).
4. Optional: further exclusive density only if a new measured residual appears under CN.

---

## Permanent constraints (every phase)

1. High performance sacred; CoreNEURON is a **guide**, not law.  
2. Heap-free: `weight_index` only; no host NET_RECEIVE **body** on native path.  
3. No host `vec_rhs` → host voltage → push `vec_v` as primary fix.  
4. Ringtest 688@100 and Traub QUALIFIED / 4474 stay green if shared paths change.  
5. Commit locally; **do not push** unless user asks.

---

## Starting prompts (copy-paste)

### P0 — triage (start here)

```text
Read ~/neuron/notes/PORTFOLIO.md (GPU-native), then
~/neuron/nrngpu/doc/gpu/native-coreneuron-parity.md (Phase 0 only),
GROK-GPU-NATIVE.md, and AGENTS.md.

Kind: feature. Portfolio: GPU-native. Phase: P0 triage.
Tree: ~/neuron/nrngpu. Branch: living GPU tip (local/gpu-native-net-soa or current tip).

Do Phase 0 only: classify ctest failures A–D; reconcile ringtest harness vs ctest;
fix cheap env/install false reds only. Fill Status tables in native-coreneuron-parity.md.
Do not start P1 feature implementations. Commit locally without push.
After open: /rename GPU-P0-triage
```

### P1 — product matrix (after P0 Status done)

```text
Read ~/neuron/notes/PORTFOLIO.md (GPU-native), then
~/neuron/nrngpu/doc/gpu/native-coreneuron-parity.md (Phase 1 Status table),
GROK-GPU-NATIVE.md, and AGENTS.md.

Kind: feature. Portfolio: GPU-native. Phase: P1 product matrix.
Tree: ~/neuron/nrngpu. Continue the first red row in the P1 Status table
(cluster order: spikes → events → state → control).

Green CoreNEURON *_py_gpu control before native. High performance sacred;
no full SoA pull as default fix. Keep ringtest 688 / Traub 4474 green.
Update Status in native-coreneuron-parity.md; commit without push.
/rename GPU-P1-<cluster>   # e.g. GPU-P1-spikes
```

### Hygiene only (bucket C noise)

```text
Read doc/gpu/native-coreneuron-parity.md failure table (bucket C only).
Kind: feature hygiene (or platform if pure CI). Fix non-native ctest noise only.
Do not expand into P1 native features. Commit without push.
/rename GPU-hygiene
```

### Resume mid-phase (generic)

```text
Read doc/gpu/native-coreneuron-parity.md — continue at first incomplete Status row
for the current phase. Same constraints as that phase's starting prompt.
Update Status before exit; commit without push.
```

### P4 — exclusive-GPU density (default next after multi-rank closed)

```text
Read ~/neuron/notes/PORTFOLIO.md (GPU-native), then
~/neuron/nrngpu/doc/gpu/native-coreneuron-parity.md (Phase 4 Status / Residual / Next),
GROK-GPU-NATIVE.md, AGENTS.md.

Kind: feature. Portfolio: GPU-native. Phase: P4 exclusive-GPU density.
Tree: ~/neuron/nrngpu. Branch: off living tip local/gpu-native as
local/gpu-p4-density (or similar explor name). Do not merge to tip until measured win.

Context: gap scatter + phase timers + multi-rank MPS policy are on tip. Ringtest
exclusive still ~2× CN; bottleneck setup-tree-matrix + lastpart-nonvint (not gap).
Dentate exclusive already ~CN; multi-rank uses CUDA MPS (not this session).

This session: one hypothesis at a time for CURRENT/STATE launch density (or related
measured residual). High performance sacred; CoreNEURON is a guide; heap-free
weight_index. Not Traub use_gap=1; not multi-rank unless blocked.

Commit locally without push. Update Status/Next before exit.
/rename GPU-P4-density
```

---

## Next (one line — update every session end)

**Next:** dentate GC — **post_solve NaN V from step 2** (prcellstate phases @ t=0.05: post_setup finite, post_solve all NaN). Exp2Syn deliver path OK after finalize-before-teardown. Harness: `prcellstate_gc_native.sh`. Kin closed. Phase C / slim JACOB parked. **Not** Traub gap.

### Starting prompt — Session A residual (closed; archive)

Session closed 2026-08-03: force-inline STATE rates; state_hh ~19 µs ≈ CN; wall multi-warm ~1.50–1.60 s; 688 green; copy tax under state_hh gone.

### Starting prompt — tip-merge H4 packet (closed; archive)

Session closed 2026-08-03: FF-merge H4 packet onto `local/gpu-native`; tip re-smoke state_hh ~19 µs, multi-warm ~1.50–1.54 s, 688 green.

### Starting prompt — Session B CURRENT (closed; archive)

Session closed 2026-08-03 on explor `local/gpu-P4-hotpath-current`: force-inline thin CURRENT; `cur_hh` ~14 µs ≈ hand; wall multi-warm ~1.31–1.37 s (clear win vs tip ~1.50–1.54); 688 green. Tip-merge justified.

### Starting prompt — tip-merge Session B (closed; archive)

Session closed 2026-08-03: FF-merge Session B onto `local/gpu-native`; tip re-smoke cur_hh ~15 µs, state_hh ~19 µs, multi-warm ~1.29–1.31 s, 688 green.

### Starting prompt — Phase C NET_RECEIVE PP (closed explor; archive)

Session closed 2026-08-03 on explor `local/gpu-P4-hotpath-netreceive`: min-present net_buf; kernel win; wall flat. **No tip-merge.**

### Starting prompt — exclusive residual / slim JACOB (closed explor; archive)

Session closed 2026-08-03 on explor `local/gpu-p4-exclusive-residual`: re-profile post H4+B — native ≈ CN; setup-tree ~0.54 s; slim JACOB wall flat; no tip-merge.

### Starting prompt — Session E setup-rhs density (closed explor; archive)

Session closed 2026-08-03 on explor `local/gpu-p4-setup-rhs-density`: phase-boundary fences only (H1 + zero/transform wait elide). Product 688 green. setup-rhs ~0.24; wall multi-warm settled ~1.18–1.28 s.

### Starting prompt — tip-merge Session E (closed; archive)

Session closed 2026-08-03: FF-merge Session E onto `local/gpu-native`; tip re-smoke product 688 green; setup-rhs ~0.24; wall multi-warm settled ~1.15–1.17 s.

### Starting prompt — multi-rank CUDA MPS (closed; archive)

Session closed 2026-08-03: product multi-rank MPS harness (`ensure_cuda_mps.sh` +
ringtest/dentate ctests); docs; Eigen STATE full-present (dentate crash → psolve
~1.5–1.6 s with MPS). Ringtest 2-rank 688 green. Residual: dentate spikes 390 vs 400.

### Starting prompt — Eigen v_unused / kin-native (closed; archive)

Session closed 2026-08-03: Eigen STATE present+refresh v_unused for functors.
testcorenrn_kin_native green. Ringtest 688@100 + 2-rank MPI green. Dentate still
390 vs 400 = all GC silent (na8st residual).

### Starting prompt — next (dentate GC post_solve NaN V)

```text
Read ~/neuron/notes/PORTFOLIO.md (GPU-native), then
~/neuron/nrngpu/doc/gpu/native-coreneuron-parity.md
  (Phase 4 Status residual: GC post_solve NaN V step 2),
GROK-GPU-NATIVE.md, AGENTS.md.

Kind: feature. Portfolio: GPU-native. Phase: P4 dentate GC post_solve NaN.
Tree: ~/neuron/nrngpu. Branch: local/gpu-native.

Context: Exp2Syn deliver OK after finalize_psolve_download before token
teardown. 390 vs 400 remains. First phase break: t=0.05 post_solve —
post_setup V finite, post_solve all 161 GC node V = NaN (CPU finite).
Step 1 t=0.025 all phases OK. Harness:
  test/external/reduced_dentate/prcellstate_gc_native.sh 500006 0.05 1
  workdir: build-gpu/test/reduced_dentate_native/neuron_gpu_native + ACC special.
Trace device post_solve / voltage update / permute-2 on GC. Not Traub gap.

This session: GPU GC voltages finite through post_solve; 400 spikes.
High performance sacred; heap-free weight_index.

Commit locally without push. Update Status/Next before exit.
/rename GPU-P4-dentate-post-solve-nan
```

### Branching (2026-08-03)

| Branch | Role |
|--------|------|
| **`local/gpu-native`** | Living tip — H4 + Session B CURRENT + **Session E** setup-stream + scatter/timers/MPS |
| `local/gpu-p4-setup-rhs-density` | Exploratory archive: Session E setup-stream density (**merged to tip**) |
| `local/gpu-p4-exclusive-residual` | Exploratory: re-profile + slim JACOB (wall flat) |
| `local/gpu-P4-hotpath-netreceive` | Exploratory: Phase C min-present net_buf (wall flat) |
| `local/gpu-P4-hotpath-current` | Exploratory archive: Session B CURRENT force-inline (**merged to tip**) |
| `local/gpu-native-net-soa` | Historical integration name; keep until remotes/docs catch up (may lag tip) |
| `local/gpu-p4-gap-phase-ab` | Exploratory archive: A+B only (pre-scatter baseline) |
| `local/gpu-p4-gap-scatter` | Exploratory archive: bulk scatter parent of tip cherry-picks |
| `local/gpu-p4-lastpart-ab` | Exploratory archive: lastpart sub-buckets (now on tip) |
| `local/gpu-p4-setup-nonvint-density` | Exploratory: defer per-mech ACC wait H1 pre-H4 (flat; superseded by Session E) |
| `local/gpu-P4-density-H2` | Exploratory: slim present/deviceptr (flat) |
| `local/gpu-P4-density-H3` | Exploratory: H3 profile only → state_hh |
| `local/gpu-P4-density-H4` | Exploratory archive: H4a–c + Session A force-inline (**merged to tip**) |
| `local/gpu-p4-multirank-share` | Exploratory archive: MPS diagnosis (landed on tip) |

P4 on tip: scatter, timers, **multi-rank MPS product harness**, Eigen full-present +
**v_unused refresh**, **H4 density packet**, **Session B CURRENT**, **Session E**.
Exclusive ringtest **≲ CN**. Multi-rank MPS **closed**. Kin-native **closed**.
**Default next:** dentate GC / na8st (390 vs 400).
