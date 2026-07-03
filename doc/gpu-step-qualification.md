# GPU-native fixed-step qualification

This document defines what it means for a NEURON model to run a **qualified**
GPU-native fixed timestep, how that differs from transitional mixed CPU/GPU
execution, and how we extend capability model-by-model (ringtest → Traub → …).

**North star:** one coherent step on device — same conceptual contract as
CoreNEURON GPU — not spike parity across every intermediate host/device partition.

---

## 1. Goals and non-goals

### Goals

- **Performance:** GPU path should not synchronise host and device each step for
  mechanisms that already live in SoA on device.
- **Correctness:** A qualified run must not split a single mechanism across host
  CURRENT and device STATE (or host Jacobian and device diagonal) in the same step.
- **Incremental delivery:** Add OpenACC/NMODL registration per mechanism class;
  qualify models as their mechanism sets become complete.
- **Diagnostics:** `prcellstate` checkpoints at named step phases are the
  primary parity tool; spike counts are a downstream check once phase state matches.

### Non-goals

- **Mixed-mode spike parity** between NEURON CPU and GPU-native when the model
  is not qualified. Mixed execution is scaffolding, not a ship target.
- **Matching CoreNEURON GPU performance** on day one for Traub-scale models.
- **CVode / variable-step GPU** (out of scope here).

---

## 2. Execution modes

| Mode | When used | Spike parity expected? |
|------|-----------|------------------------|
| **CPU** | Default; model not qualified | Reference |
| **GPU-qualified** | All gates pass (§4) | Yes (prcellstate first, then spikes) |
| **GPU-transitional** | `enable_gpu=1` but gates fail | **No** — dev only; may warn or refuse |

Transitional mode exists while infrastructure is built. It must not be used as
a benchmark or regression baseline.

**Direction:** move Traub from transitional → qualified by registering device
phases for mechanisms, not by adding host↔device sync patches.

---

## 3. Fixed-step phase order (NEURON GPU-native)

One thread (generalisation to multi-thread is the same gates per thread).

```text
┌─ step N begin (t = t_{N-1}) ─────────────────────────────────────────────┐
│  [optional] sync voltages for threshold / vecplay                          │
│  deliver_net_events  →  check_thresh (device if qualified)                 │
│  advance t += 0.5·dt; vecplay                                              │
├─ setup_tree_matrix ────────────────────────────────────────────────────────┤
│  BEFORE_BREAKPOINT; zero rhs/d; mechanism CURRENT; axial rhs               │
│  mechanism JACOBIAN + capacitance; axial lhs                               │
│  prcellstate: post_setup                                                   │
├─ matrix solver (device when rhs/d on device) ──────────────────────────────┤
├─ post_solve ─────────────────────────────────────────────────────────────┤
│  second_order_cur (if secondorder=2); update V                             │
│  prcellstate: post_solve                                                   │
├─ lastpart (nrn_fixed_step_lastpart) ─────────────────────────────────────┤
│  gap / scatter if configured                                               │
│  prcellstate: pre_nonvint                                                  │
│  nonvint: mechanism STATE (+ LONGITUDINAL diffusion)                       │
│  prcellstate: post_nonvint                                                 │
│  AFTER_SOLVE; fixed_record; deliver_post_step_events                       │
│  advance t += 0.5·dt  →  end of step (t = t_N)                             │
└────────────────────────────────────────────────────────────────────────────┘
```

Implementation entry: `neuron::gpu::fixed_step_thread()` in
`src/neuron/gpu/fadvance_gpu.cpp`, then `nrn_fixed_step_lastpart()` in
`src/nrnoc/fadvance.cpp`.

---

## 4. Qualification gates

A model is **GPU-qualified** when all of the following hold for every active
`NrnThread` with `nt.end > 0`. Inspect at runtime via HOC:

```hoc
pc.gpu_fixed_step_phases()
```

Implementation: `neuron::gpu::native_gpu_fixed_step_phase_report()` in
`src/neuron/gpu/mechanism_phases.cpp`.

### Gate A — Matrix storage on device

`matrix_rhs_d_stays_on_device_for_solve(nt) == true`

Blocked by: `use_sparse13`, extracellular nodes (`_ecell_memb_list`),
Python nonvint block.

### Gate B — CURRENT + JACOBIAN on device (fast matrix path)

`matrix_currents_on_device(nt) == true`

For every mechanism on the thread with a `current` / `jacob` hook:

- `mechanism_current_on_device(type)` must be true if `current` exists.
- `mechanism_jacobian_on_device(type)` must be true if `jacob` exists.
- Capacitance (`CAP`) Jacobian must be registered on device.

Registration: `register_mechanism_gpu_phases(type, …)` from NMODL OpenACC
codegen or built-in code (e.g. `capac.cpp`, `stim`).

When Gate B fails, host CURRENT/Jacobian still run and rhs/d are pushed to
device before solve — **transitional mixed path**.

### Gate C — STATE (nonvint) on device

`solve_phase_on_device(nt) == true`

Requires:

- `NRN_NATIVE_GPU_DEVICE_NONVINT=1` (or successor build-time default when
  ready), and
- Every mechanism with a `state` hook has `mechanism_solve_on_device(type)`.

When Gate C fails, STATE must run on **host** with `compute_gpu=0` for nonvint
(see §6.2). OpenACC STATE kernels must not update device-only copies while host
CURRENT reads host SoA.

### Gate D — post_solve on device

`!post_solve_needs_host_fallback(nt)`

Blocked by sparse13 and extracellular (same as Gate A in practice).

Qualified path: `post_solve_on_device()` (device `second_order_cur`, voltage
update, fast imem).

### Gate E — Threshold detection on device

All threshold `PreSyn` instances use modern SoA voltage handles and
`check_thresh_presyn_on_device()` runs with current device `vec_v`.

Host `PreSyn::check` is skipped only when the device path reports success.

### Gate F — lastpart without host SoA pull (stretch)

`!lastpart_host_phases_required(nt)`

Qualified lastpart should not require `begin_lastpart_host_phases()` to pull
full node/mechanism SoA for AFTER_SOLVE / `fixed_record` / HOC callbacks.

**Current state:** lastpart host phases still run for many native-GPU builds.
Treat Gate F as a **ringtest-first** target; Traub may follow after Gates B–C.

### Summary predicate (proposed API)

```cpp
bool model_qualifies_for_full_gpu_native(NrnThread const& nt) noexcept;
```

Suggested definition (all required):

- Gate A ∧ Gate B ∧ Gate C ∧ Gate D ∧ Gate E
- Gate F optional until lastpart is device-resident

---

## 5. Per-mechanism registration

| `MechanismGpuPhase` | NMODL / runtime | Fixed-step use |
|---------------------|-----------------|----------------|
| `Current` | BREAKPOINT current | `nrn_rhs` |
| `Jacobian` | `nrn_jacob` | `nrn_lhs` |
| `Solve` | BREAKPOINT SOLVE / STATE | `nonvint` |

Codegen: `codegen_neuron_acc_visitor.cpp` emits registration after `mech_type`
assignment. Built-in mods (capacitance, `stim`, …) register from C++.

**Rule:** If a mechanism is active in the model and has a hook, either register
the corresponding device phase or the model remains **unqualified**.

---

## 6. Transitional vs qualified behaviour

### 6.1 Qualified (target)

- No per-step `sync_matrix_arrays_to_device` after host mechanisms.
- No host `nonvint` with `compute_gpu=1` on OpenACC mods.
- Threshold: device kernel only; hysteresis flags synced as needed.
- Optional `gpu_download_flush` for HOC readback — not on the integration hot path.

### 6.2 Transitional (current Traub)

Explicit hazards (do not certify):

| Hazard | Symptom | Mitigation until qualified |
|--------|---------|---------------------------|
| Host CURRENT, device STATE | Frozen `m`/`h`, wrong dynamics, 0 or excess spikes | Force `compute_gpu=0` in `nonvint` when Gate C fails (`lastpart.cpp`) |
| Host + device Jacobian | Wrong `vec_d`, voltage drift | GPU Jacobian only when Gate B; else host Jacobian only |
| Device threshold, stale `vec_v` | Missing spikes | Sync V before check; host fallback if device path fails |
| Duplicate `second_order_cur` | Ion current drift | Run once per step (device post_solve *or* host lastpart, not both) |

Transitional mitigations are **safety nets**, not architecture.

---

## 7. Model roadmap

### Tier 0 — Ringtest

**Purpose:** Close the qualified loop on a minimal mod set.

| Check | Command / tool |
|-------|----------------|
| Phase report | `pc.gpu_fixed_step_phases()` — all Gates A–E yes |
| prcellstate | Short run, one gid, steps 0–2: `post_setup`, `post_solve`, `post_nonvint` |
| Spikes | ringtest ctest / `out*.dat` vs CPU |
| Performance | Wall time vs NEURON CPU (expect win when qualified) |

Mods: pas, hh, expsyn, stim/IClamp, capacitance — built-in OpenACC codegen in
`src/nrniv/CMakeLists.txt`. Ion mechanisms (`*_ion`) are bookkeeping only and
are excluded from Gate B. Gate C requires `NRN_NATIVE_GPU_DEVICE_NONVINT=1`.

### Tier 1 — Traub (82894)

**Purpose:** Force honest gaps at scale.

1. Run `pc.gpu_fixed_step_phases()` after `init.hoc` — capture blocking mech lists.
2. prcellstate on **gid 171** (or chosen cell), short `mytstop`, step triggers
   around first CPU spike (~step 56–57 at dt=0.025).
3. Register ion/channel mods on device (naf, kdr, …) — CURRENT + Jacobian + Solve.
4. Re-run phase report until Gate B and Gate C pass.
5. Spike comparison only after prcellstate parity at `post_nonvint`.

Do **not** block Traub work on mixed-mode spike CI.

### Tier 2+ — Gap junction, extracellular, ARTIFICIAL_CELL, …

Each feature either gains a device implementation or remains a qualification
blocker with a clear report line.

---

## 8. Diagnostics (prcellstate)

### Checkpoint phases

| Phase | When | Typical use |
|-------|------|-------------|
| `post_setup` | After `setup_tree_matrix` | rhs/d, mechanism CURRENT |
| `post_solve` | After voltage update | Node `v`, consistency with threshold |
| `pre_nonvint` | Before `nonvint` | V synced for STATE |
| `post_nonvint` | After STATE | Channel `m`/`h`/ions; **primary STATE parity** |

### Usage (HOC)

```hoc
// Arm before prun/psolve — step index 0-based from psolve start
prcellstate_gid = 171
prcellstate_checkpoint = 56   // or pc.prcellstate_checkpoint(gid, -1, 1.425) for time
```

Output: `<gid>_<backend>_s<step>_<phase>.nrndat` (see `prcellstate_checkpoint.cpp`).

### Comparison workflow

1. CPU reference run (`enable_gpu=0`) → `171_cpu_s56_post_nonvint.nrndat`
2. GPU run (`enable_gpu=1`) → `171_gpu_s56_post_nonvint.nrndat`
3. Compare node `inode v` and mechanism blocks (e.g. `naf` fields at soma inode).
4. First phase where CPU/GPU diverge → that fixed-step phase needs device work.

Spike files (`out1.dat`) are step 5, not step 1.

---

## 9. CI and benchmarks

| Target | Requirement |
|--------|-------------|
| Ringtest GPU-qualified | Gates pass; prcellstate + spikes vs CPU |
| Traub GPU-transitional | Build + run allowed; **no spike count match** |
| Traub GPU-qualified | Gates pass; prcellstate on gid 171; then spike tolerance TBD |

`run_benchmark.py` should label runs: `qualified` vs `transitional` vs `cpu`.

---

## 10. Relationship to CoreNEURON GPU

| Aspect | CoreNEURON GPU | NEURON GPU-qualified |
|--------|----------------|----------------------|
| SoA layout | Yes | Yes (`neuron::model`) |
| NMODL OpenACC | Yes | Same codegen path (NEURON visitor) |
| Step orchestration | CoreNEURON fadvance | `fixed_step_thread` + lastpart |
| Permute / cell order | Required for perf | Interleave optional; same gates |

CoreNEURON remains the performance reference for large models until NEURON native
qualifies the same mechanism set.

---

## 11. Implementation checklist (next engineering)

1. **`model_qualifies_for_full_gpu_native()`** — single predicate from §4.
2. **Policy at `psolve` start:** warn, or refuse GPU, if `enable_gpu=1` and not qualified.
3. **Extend phase report** with explicit `QUALIFIED: yes/no` line.
4. **Ringtest:** satisfy Gates A–E; document in ctest README.
5. **Traub:** OpenACC + registration for blocking mechs from phase report.
6. **Remove transitional sync** as gates clear (not before).

---

## 12. References (code)

| Topic | Location |
|-------|----------|
| GPU step driver | `src/neuron/gpu/fadvance_gpu.cpp` |
| Phase report | `src/neuron/gpu/mechanism_phases.cpp`, `pc.gpu_fixed_step_phases()` |
| Gate B predicate | `src/neuron/gpu/sync.cpp` — `matrix_currents_on_device()` |
| Mechanism registry | `src/neuron/gpu/mechanism_phases.hpp` |
| nonvint / lastpart | `src/nrnoc/fadvance.cpp`, `src/neuron/gpu/lastpart.cpp` |
| Threshold | `src/neuron/gpu/check_thresh.cpp`, `src/nrncvode/netcvode.cpp` |
| prcellstate checkpoints | `src/nrniv/prcellstate_checkpoint.cpp` |
| NMODL registration emit | `src/nmodl/codegen/codegen_neuron_acc_visitor.cpp` |

---

*Status: strategy document — mixed-mode parity explicitly deprecated as a goal.
Last updated: 2026-07-03.*