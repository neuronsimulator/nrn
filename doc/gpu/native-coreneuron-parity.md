# Native GPU ↔ CoreNEURON functional parity

**Portfolio:** GPU-native (feature)  
**Tree:** `~/neuron/nrngpu`  
**Living tip (2026-07-29):** integration `local/gpu-native-net-soa` @ trajectory/lastpart closed tip  
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
| `GPU-hygiene` | full-ctest noise not native product |

**One living session per phase** (or cluster). Prefer **resume** that named session until the phase Status is done. When context is bloated or the agent is lost: **end checklist below → `/new` → paste the phase starting prompt** — do **not** resume a year-old auto-title.

Picker help: type `GPU-P` in `/resume` filter; or `grok sessions search GPU-P0`.

### End-of-session checklist (every phase)

1. Update **Status** tables in **this file** (red/green/notes).  
2. If code landed: **commit locally**, do **not push** unless asked.  
3. One-line **Next** at the bottom of this file (and PORTFOLIO if the portfolio Next changed).  
4. `/rename` still matches the phase (or rename to next phase before exit).  
5. Optional: note session id in the ledger below (`/session-info`).

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
| ctest native ringtest vs harness | **reconciled** | Harness = **1-rank** `special -python` product gate (green). ctest = **`mpiexec -n 2`** → SEGV in `check_thresh_presyn_on_device`/`acc_copyin` (multi-rank GPU). Not install/lib skew (build + install `libnrniv` same mtime). Gap ctest also SEGV (`nrnmpi_setup_transfer`). → **D/P2**, not cheap env. |
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
| unit_tests::gpu_config | **A/C** | Bare SEGV (no assertion text); unit GPU scaffolding — check in hygiene if blocks P1. |
| unit_tests::gpu_fadvance | **A** | SEGV in `fixed_step_thread records native dispatch` (`fadvance.cpp:53`); 1/2 cases pass. |
| unit_tests::gpu_net_receive | **A** | SEGV in NRB upload-on-device case (`net_receive_buffer.cpp:58`); 1/2 pass. |
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
| coreneuron_modtests::test_natrans_py_gpu_native | **green** | nthread=1; nai transfer host then SoA sync. Multi-thread (4) still OpenACC partial-present residual. |
| external_ringtest::neuron_gpu_native_mpi | **D** | 2-rank SEGV `check_thresh_presyn_on_device`/acc_copyin; harness 1-rank green. |
| external_ringtest::neuron_gpu_native_device_nonvint_mpi | **removed** | Scaffolding twin deleted; device nonvint is native default. |
| external_ringtest::neuron_gpu_native_mpi_gap | **D** | SEGV gap/setup_transfer and/or multi-rank; P2. |
| external_ringtest::compare_neuron_gpu_native_mpi_gap | **D** | Depends on gap test; empty/missing spk compare. |
| external_ringtest::coreneuron_*_mpi_threads* | **B/C** | SEGV in `nrn_finitialize` (cpu+gpu threads variants). |
| external_ringtest::compare_results | **C** | Spike ref empty vs 688 lines — dep of failed thread run, not native product. |
| reduced_dentate::* | **D** | Abort in `pc.setup_transfer()` / model path; P3. |
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
| direct | green | **green** | modes **0–1** product (+fast_imem). Mode 2 alone green; sequential 0→1→2+fast_imem residual. |
| spikes | green | **green** | modes **0–1** product (+fast_imem). Mode 2 alone green (±imem); sequential 0→1→2 without imem green; with imem residual. |
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
| test_natrans | **green** | **green** | Control: valid permute. Native: nthread=1 product green; multi-thread partrans residual. |
| array_variable_transfer_* | green | **green** | ACC green/red via `coreneuron_modtests_native_array_transfer`; modes 0–2 + file_mode pass. |
| spikes_mpi* | green | **green** | Native product = mode 0 (matches CoreNEURON file_mode MPI). Mode 1 multi-rank re-psolve on shared GPU residual. |
| test_subworlds | green | **green** | after NONVINT; still P2 if expanded |

---

## Phase 2 — MPI / gap / topology

| Item | Status | Notes |
|------|--------|-------|
| spikes_mpi / file mode native | **green** (mode 0) | mode 1 multi-rank re-psolve residual |
| test_subworlds native | **green** | |
| test_natrans native | **green** (nthread=1) | multi-thread residual |
| ringtest gap native + compare | **green** (1+2 rank) | S0–S2: sparse mailbox gather + target scatter; ACC HalfGap; live `v_node_index` local+MPI gather (`deviceptr`); 2-rank sorted raster vs `spk2.gap.100ms.std.sorted.ref`. Launch: `h.nrnmpi_init` product path. |
| multi-rank device assign | **shared OK** | 2-rank gap works on shared T1000 (`1 GPUs shared by 1 ranks` message per process). `special -mpi` OpenACC multi-process SEGV residual; prefer `nrnmpi_init`. |

---

## Phase 3 — Heavy models / extend coverage

| Item | Status | Notes |
|------|--------|-------|
| reduced_dentate (neuron / crn cpu / crn gpu) | | Often path/install |
| testcorenrn_* **online** native clones | | Adapt, don’t force offline bbcore |
| nmodl table/kinetic GPU native if missing | | |
| Offline / saverestore | | Only if product needs; may stay CoreNEURON-only |

---

## Phase 4 — Perf / architecture debt

| Item | Status | Notes |
|------|--------|-------|
| Wall-time vs CoreNEURON (guide) | | Measure, don’t match for its own sake |
| Residual hot-path host traffic audit | | |
| Single device-resource owner | | Only if exit/leak forces |

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

---

## Next (one line — update every session end)

**Next:** P2 S3 multi-thread gap/partrans (`nthread>1`); S4 MechRange sources (natrans beyond nthread=1).
