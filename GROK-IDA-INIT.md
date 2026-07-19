# Grok handoff: IDA consistent initialization (`nrnida`)

Use this file when starting a **new** Grok session rooted in `~/neuron/nrnida`.

---

## Status (2026-07)

| Phase | Status | Notes |
|-------|--------|--------|
| **0** Mode API + `IDA_Y_INIT` plumbing | **Done** | `CVode.dae_init_mode` 0–2; default still heuristic (0) |
| **R** Industry / circuit reinit survey | **Done** | `~/neuron/notes/ida_phase_R_industry_map.md` |
| **Battery IC** (mode **3**) LM caps | **Done** | C→V hold \(\Delta v\); algebraic solve |
| Mode 3 inductors + op-amp \(\tau>0\) | **Done** | Diagonal L hold; hold \(v_k\), drop lag row |
| Mode 3 extracellular membrane/layers | **Done** | Hold \(V_m\), \(xc>0\) drops; outer algebraic free |
| Soft sparse13 factor fail | **Done** | Mode 1/3 can fall back without abort |
| **Three-panel IC audit** | **Done** | `dae_init_audit` / `dae_init_audit_file`; panel A = retreat residual |
| Automated tests | **Partial** | `test/hoctests/tests/test_ida_init_mode.py` (isolated subprocesses) |
| Manual GUI circuits | **Local** | `external/tests/nrntest/nrniv/ida/*.ses` (tree is **gitignored**) |

**Default remains mode 0.** Mode 3 is experimental; falls back to heuristic on project failure.

Tip commit (update when advancing): see `git log -1 --oneline` on `hines-grok/ida-init`.

---

## Why this worktree

Focused DAE/IDA initial-condition work. Branch from **master**; do not revive permanent VMX / `v12` state doubling from historical `origin/idainit`.

| Worktree / path | Branch | Purpose |
|-----------------|--------|---------|
| `~/neuron/nrnida` | `hines-grok/ida-init` | IDA IC modes, battery project, audit |
| (notes) `~/neuron/notes/` | n/a | Design notes outside the repo |

---

## North star (success criteria)

After `finitialize` and after each discontinuity (`NET_RECEIVE`, `Vector.play`, `at_time`, clamps, …):

1. **Residual:** \(g(y', y, t) = 0\) within integrator WRMS / ewt tolerance.
2. **Physical \(dt\to 0\):** hold continuous content; free absolute algebraics as network requires.
   - **Hold:** ODE states; capacitor \(\Delta v\) / charge; inductor current; membrane \(V_m\) with extracellular; \(xc>0\) layer drops.
   - **Free to jump:** absolute node voltages (e.g. series \(C\)–\(R\) + \(I\) step).

Nano-\(dt\) fully implicit Euler is a **practical projector and fallback**, not the definition of (2).

### Non-goals

- Permanent `USE_VMX` / doubled continuous state for integration (`origin/idainit`).
- Block on SUNDIALS 3 / PR #1960 (`slds`) for IC design (revalidate later).
- Pure speculative \(g_{\mathrm{ic}}\) stacks without industry map (Phase R already done).

---

## Architecture (current)

```text
Discontinuity / finitialize
    │
    ▼
Daspk::init()  [src/nrncvode/nrndaspk.cpp]
    │  mode 0 → heuristic nano-step (dae_init_dteps)
    │  mode 1 → IDACalcIC(IDA_Y_INIT) then heuristic fallback
    │  mode 2 → IDA_Y_INIT only
    │  mode 3 → battery project then residual check; fallback heuristic
    ▼
Cvode::res  →  G ≈ C y' − f(y)  (membrane cm, xc layers, nrndae_dkres)
```

| Piece | Location |
|-------|----------|
| IC modes / audit | `src/nrncvode/nrndaspk.{h,cpp}` |
| HOC API | `src/nrncvode/cvodeobj.cpp` (`dae_init_mode`, `dae_init_dteps`, `dae_init_audit`, `dae_init_audit_file`) |
| Battery project (LM) | `src/nrniv/linmod.cpp` `battery_ic_project()` |
| NrnDAE entry | `src/nrniv/nrndae.cpp` `nrndae_battery_ic_project()` |
| Extracellular battery | `src/nrnoc/extcelln.cpp` `nrn_extracellular_battery_ic()` |
| Soft factor fail | `src/nrnoc/solve.cpp` + `nrn_sparse13_soft_fail` |
| Docs | `docs/progref/simctrl/cvode.rst` |
| Tests | `test/hoctests/tests/test_ida_init_mode.py` |
| Design notes | `~/neuron/notes/ida_y_init_adoption.md`, `ida_phase_R_industry_map.md` |

### Battery IC (mode 3) idea

Hold continuous content by temporary stamp replacement:

- Floating caps → voltage sources holding \(\Delta v\)
- Diagonal mass (inductor) → hold current state
- OpAmp lag \(C[o][k]=\tau\) → hold \(v_k\), drop dynamic row
- Extracellular: hold \(V_m\) and capacitive layer drops

### Three-panel audit

```text
CVode().dae_init_audit(2, T)     # level 0–2; arm first reinit with t >= T (one-shot)
CVode().dae_init_audit_file("ic_audit.txt")  # append; empty → stdout
```

| Panel | Meaning |
|-------|---------|
| **A pre** | Continuous \((y,y')\) + residual at **integrator retreat** (`interpolate` after possible overshoot). Continuous play is at event \(t\). Do **not** re-eval after the jump. |
| **B post-event pre-IC** | After discontinuity, before projector |
| **C post-IC** | After mode 0/1/2/3 path |

Retreat applies to `Vector.play`, `NetCon`/`NET_RECEIVE`, and `at_time` (same stop → interpolate → deliver → reinit path).

Capture: `audit_save_pre_from_delta()` after `res` in `advance_tn` and **overwrite** in `interpolate`.

---

## Recommended next work

1. **Validate audit** interactively on `iramp1.ses` / `iramp5.ses` (audit at ramp start/end): A at event \(t\), residual ~0; B broken; C clean.
2. **Stringent multi-cap case:** `wheatstone1.ses` under `external/tests/nrntest/nrniv/ida/` (local/gitignored) or automate a wheatstone LM in pytest.
3. **Event-reinit automated tests** (not only `finitialize`) for mode 3 continuous content.
4. **Mode 3 hardening / policy:** when to recommend over 0 for LinearCircuit / extracellular; open layer-0 + LM; multi-layer edge cases; diagnostics/fallback visibility.
5. Update notes last-updated / tip commit when status drifts.
6. Later: SUNDIALS 3 revalidation; do **not** resurrect permanent VMX.

---

## Build

Typical local build dir used in prior sessions: `~/neuron/nrnida/build-ida-init/` (not committed).

```bash
cd ~/neuron/nrnida
mkdir -p build-ida-init && cd build-ida-init
cmake .. -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DNRN_ENABLE_TESTS=ON \
  -DNRN_ENABLE_MPI=OFF   # match your usual prefs
ninja -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)
# GUI circuits need nrngui / share path from this build
```

---

## Tests

```bash
cd build-ida-init   # or your build tree
export PYTHONPATH="$PWD/lib/python${PYTHONPATH:+:$PYTHONPATH}"
export DYLD_LIBRARY_PATH="$PWD/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"  # macOS
# Linux: LD_LIBRARY_PATH similarly

python ../test/hoctests/tests/test_ida_init_mode.py
# or pytest path used by ctest for hoctests if configured
```

Manual (requires `external/tests` checkout and GUI build):

```bash
cd external/tests/nrntest/nrniv/ida   # if present
nrngui iramp1.ses
# then: cvode.dae_init_mode(3)  cvode.dae_init_audit(2, 2)  Init&Run
```

---

## Starting prompt (new session)

```
Read ~/neuron/nrnida/GROK-IDA-INIT.md and, if present,
~/neuron/notes/ida_y_init_adoption.md and ida_phase_R_industry_map.md.

Repo: ~/neuron/nrnida, branch hines-grok/ida-init (from master).
Build: prefer existing build-ida-init/ or reconfigure as in the handoff.

Goal: consistent IDA IC after finitialize and discontinuities —
residual ~0 AND physical dt→0 (hold continuous content).

Done: dae_init_mode 0–3 (0 default heuristic; 3 experimental battery IC
for LM C/L/opamp + extracellular), soft singular J, three-panel audit
with panel A = continuous residual at integrator retreat (interpolate).

Do not revive permanent VMX/v12. Default stays mode 0 until mode 3 is
validated.

Next: (1) smoke-test audit on iramp* at event times; (2) event-reinit
automated tests / wheatstone multi-cap; (3) mode 3 policy and edge
cases — or ask what to prioritize.
```

---

## Historical context

| Branch / path | Lesson |
|---------------|--------|
| `origin/idainit` | `IDA_YA_YDP_INIT` + permanent v12/VMX; LinearCircuit better; extracellular never fully worked; doubled state |
| Pure `IDA_Y_INIT` (mode 1/2) | Algebraic LM OK; folded caps → singular \(\partial g/\partial y\) at \(c_j=0\) |
| Nano-step heuristic | Makes \(\|g\|\) small by finite \(C/\mathrm{d}t\); smears jump over \(\varepsilon\) |

Industry map (Phase R): hold differentials / content; solve algebraics — aligns with battery IC approach (SPICE-like IC holds, switched-DAE reinits).
