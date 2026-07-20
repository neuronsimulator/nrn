# Grok handoff: IDA consistent initialization (`nrnida`)

Use this file when starting a **new** Grok session rooted in `~/neuron/nrnida`.

---

## Status (2026-07)

| Phase | Status | Notes |
|-------|--------|--------|
| **0** Mode API + `IDA_Y_INIT` plumbing | **Done** | `CVode.dae_init_mode` 0–2; default still heuristic (0) |
| **R** Industry / circuit reinit survey | **Done** | `~/neuron/notes/ida_phase_R_industry_map.md` |
| **Battery IC** (mode **3**) LM + extracellular hold | **Done** | C→V hold \(\Delta v\); L hold; op-amp \(\tau\); ext \(V_m\)/layers |
| Mode 3 **\(C y'=f\)** for \(y'\) | **Done** | Continuous limit; no `dteps` bias; nano-step only on residual fallback |
| Soft sparse13 factor fail | **Done** | Mode 1/3 can fall back without abort |
| **Three-panel IC audit** | **Done** | `dae_init_audit` / `dae_init_audit_file`; audit suppresses mode-3 fallback |
| Singular / algebraic residual diagnosis | **Done** | Top residual eqs classified (algebraic / near-singular \(c\)) on mode-3 fail |
| Automated tests | **Partial** | `test/hoctests/tests/test_ida_init_mode.py` (incl. SEClamp tiny `cm`) |
| Manual GUI circuits | **Local** | `external/tests/nrntest/nrniv/ida/*.ses` (tree is **gitignored**); `~/models/nrndc1sim` |

**Default remains mode 0.** Mode 3 is ready for broader validation; falls back to heuristic on residual failure (except when an audit is armed).

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

Nano-\(dt\) fully implicit Euler is a **fallback**, not the definition of (2). Mode 3 recovers \(y'\) from \(C y' = f(y,t)\) at fixed projected \(y\).

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
    │  mode 3 → battery y-project → y' from C*y'=f(y); residual fail → heuristic
    │            (audit armed: no fallback; diagnose top residual eqs)
    ▼
Cvode::res  →  G ≈ C y' − f(y)  (membrane cm, xc layers, nrndae_dkres)
```

| Piece | Location |
|-------|----------|
| IC modes / audit / \(C y'=f\) | `src/nrncvode/nrndaspk.{h,cpp}` |
| HOC API | `src/nrncvode/cvodeobj.cpp` (`dae_init_mode`, `dae_init_dteps`, `dae_init_audit`, `dae_init_audit_file`) |
| Battery project (LM) | `src/nrniv/linmod.cpp` `battery_ic_project()` |
| NrnDAE entry + LM \(y'\) seed | `src/nrniv/nrndae.cpp` |
| Extracellular battery | `src/nrnoc/extcelln.cpp` `nrn_extracellular_battery_ic()` |
| Soft factor fail | `src/nrnoc/solve.cpp` + `nrn_sparse13_soft_fail` |
| Docs | `docs/progref/simctrl/cvode.rst` |
| Tests | `test/hoctests/tests/test_ida_init_mode.py` |
| Design notes | `~/neuron/notes/ida_y_init_adoption.md`, `ida_phase_R_industry_map.md` |

### Mode 3 idea (productized)

1. **Project \(y\):** hold continuous content (LM floating \(C\), diagonal \(L\), op-amp lag; extracellular \(V_m\) / \(xc>0\) drops); free absolute algebraics as the network requires.
2. **Recover \(y'\):** diagonal / simple-mass solve \(C y' = f(y,t)\) (membrane \(10^{-3} c_m\), mechanism identity, LM single-column/difference stamps). **No `dteps`.**
3. **Residual check:** on failure, classify top residual eqs (algebraic \(c=0\) vs near-singular \(c\)); fall back to nano-step heuristic unless audit is armed.

`finitialize` still chooses \(y\) via `v_init` + `INITIAL` + defaults (`vext=0`). Mode 3 does **not** invent a consistent algebraic \(y\) when that recipe is wrong.

### Three-panel audit

```text
CVode().dae_init_audit(2, T)     # level 0–2; arm first reinit with t >= T (one-shot)
CVode().dae_init_audit_file("ic_audit.txt")  # append; empty → stdout
```

| Panel | Meaning |
|-------|---------|
| **A pre** | Continuous \((y,y')\) + residual at **integrator retreat** (`interpolate` after possible overshoot). Continuous play is at event \(t\). Do **not** re-eval after the jump. |
| **B post-event pre-IC** | After discontinuity, before projector |
| **C post-IC** | After mode 0/1/2/3 path (mode 3 + audit: pure mode 3 even if residual fails) |

---

## Recommended next work

1. Broader validation: `~/models/nrndc1sim`, `iramp*`, wheatstone / multi-cap LM.
2. Event-reinit automated tests (not only `finitialize`) for mode 3 continuous content.
3. Policy: when to recommend mode 3 over 0; `cm→0` / ideal clamp as algebraic; denser LM \(C\) solve.
4. Optional HOC API: IDA-side `f` / residual probe (like `CVode.f` for CVODE).
5. Update tip commit when status drifts.
6. Later: SUNDIALS 3 revalidation; do **not** resurrect permanent VMX.

---

## Build

Typical local build dir used in prior sessions: `~/neuron/nrnida/build/` or `build-ida-init/`.

```bash
cd ~/neuron/nrnida
mkdir -p build && cd build
cmake .. -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DNRN_ENABLE_TESTS=ON \
  -DNRN_ENABLE_MPI=OFF
ninja -j$(nproc)
```

---

## Tests

```bash
cd build
export PYTHONPATH="$PWD/lib/python${PYTHONPATH:+:$PYTHONPATH}"
export LD_LIBRARY_PATH="$PWD/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

python ../test/hoctests/tests/test_ida_init_mode.py
```

Manual: `~/models/nrndc1sim` with `h.cvode.dae_init_mode(3)` before `GUI()` / run.

---

## Starting prompt (new session)

```
Read ~/neuron/nrnida/GROK-IDA-INIT.md and, if present,
~/neuron/notes/ida_y_init_adoption.md and ida_phase_R_industry_map.md.

Repo: ~/neuron/nrnida, branch hines-grok/ida-init (from master).
Build: prefer existing build/ or reconfigure as in the handoff.

Goal: consistent IDA IC after finitialize and discontinuities —
residual ~0 AND physical dt→0 (hold continuous content).

Done: dae_init_mode 0–3 (0 default heuristic; 3 = battery y-hold + y' from
C*y'=f, heuristic fallback + residual diagnosis; audit suppresses fallback).

Do not revive permanent VMX/v12. Default stays mode 0 until mode 3 is
broadly validated.

Next: broader model validation; event-reinit tests; cm→0 / algebraic clamp
policy — or ask what to prioritize.
```

---

## Historical context

| Branch / path | Lesson |
|---------------|--------|
| `origin/idainit` | `IDA_YA_YDP_INIT` + permanent v12/VMX; LinearCircuit better; extracellular never fully worked; doubled state |
| Pure `IDA_Y_INIT` (mode 1/2) | Algebraic LM OK; folded caps → singular \(\partial g/\partial y\) at \(c_j=0\) |
| Nano-step heuristic | Makes \(\|g\|\) small by finite \(C/\mathrm{d}t\); \(O(\mathrm{dteps}\cdot\|y'\|)\) residual bias (fails tiny \(c_m\)) |
| Mode 3 \(C y'=f\) | Clears residual when \(y\) already on manifold (SEClamp + tiny \(c_m\) scm2eem) |

Industry map (Phase R): hold differentials / content; solve algebraics; recover \(y'\) from mass equation.
