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
| 2026-07-29 | (this discussion) | — | Plan created; no code |
| | | | |

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
   export NRN_NATIVE_GPU_DEVICE_NONVINT=1 NRN_GPU_BACKEND_TEST=native NRN_GPU_PERMUTE=2
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
| Harness ringtest short | | |
| Harness ringtest 688@100 | | |
| ctest native ringtest vs harness | | |
| Failure table filled | | |
| CoreNEURON control smokes | | |

### Failure triage table (fill in P0)

| Test name | Bucket | One-line reason |
|-----------|--------|-----------------|
| test-solver | | |
| unit_tests::benchmarks | | |
| unit_tests::gpu_config | | |
| unit_tests::gpu_fadvance | | |
| unit_tests::gpu_net_receive | | |
| ringtest | | |
| pytest_coreneuron::basic_tests_py3.14 | | |
| coverage_tests::cover_tests | | |
| hoctests::* | | |
| parallel::* | | |
| coreneuron_modtests::*_cpu / *_gpu (non-native fails) | | |
| coreneuron_modtests::*_gpu_native | | |
| external_ringtest::neuron_gpu_native* | | |
| external_ringtest::coreneuron_* (fails) | | |
| reduced_dentate::* | | |
| external_nrntest | | |
| compare_results | | |

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

| Test | Control `*_gpu` | Native `*_gpu_native` | Notes |
|------|-----------------|------------------------|-------|
| fornetcon | | | |
| direct | | | |
| spikes | | | |
| spikes_file_mode | | | |
| fast_imem | | | |
| datareturn | | | |
| test_units | | | |
| test_netmove | | | |
| test_pointer | | | |
| test_watchrange | | | |
| test_psolve | | | |
| test_ba | | | |
| test_nmodlrandom | | | |
| test_nmodlrandom_syntax | | | |
| test_natrans | | | → may slip to P2 |
| array_variable_transfer_* | | | |
| spikes_mpi* | | | → P2 |
| test_subworlds | | | → P2 |

---

## Phase 2 — MPI / gap / topology

| Item | Status | Notes |
|------|--------|-------|
| spikes_mpi / file mode native | | |
| test_subworlds native | | |
| test_natrans native | | |
| ringtest gap native + compare | | Handoff “later: use_gap=1” |
| multi-rank device assign | | unit test `gpu_device_assign_mpi` |

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

**Next:** Run Phase 0 triage session (`/rename GPU-P0-triage`); fill failure table and ringtest harness vs ctest.
