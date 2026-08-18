# Grok handoff: IDA consistent initialization (`nrnida`)

Use this file when starting a **new** Grok session rooted in `~/neuron/nrnida`.

---

## Status (2026-08-12)

| Phase | Status | Notes |
|-------|--------|--------|
| **0** Mode API + `IDA_Y_INIT` plumbing | **Done** | `CVode.dae_init_mode` 0–3; shipped default 0; `NRN_DAE_INIT_MODE_DEFAULT` overrides at registration |
| **R** Industry / circuit reinit survey | **Done** | `~/neuron/notes/ida_phase_R_industry_map.md` |
| **Battery IC** (mode **3**) LM + extracellular hold | **Done** | C→V hold \(\Delta v\); L hold; op-amp \(\tau\); ext \(V_m\)/layers |
| Mode 3 **\(C y'=f\)** for \(y'\) | **Done** | Continuous limit; no `dteps` bias; nano-step only on residual fallback |
| Soft sparse13 factor fail | **Done** | Mode 1/3 can fall back without abort |
| **Three-panel IC audit** | **Done** | `dae_init_audit` / `dae_init_audit_file`; audit suppresses mode-3 fallback |
| Singular / algebraic residual diagnosis | **Done** | Top residual eqs classified (algebraic / near-singular \(c\)) on mode-3 fail |
| Automated tests | **Partial** | `test_ida_init_mode.py` (mode 3, SEClamp, forcing suite A3, A5 stats) |
| Manual GUI circuits | **Local** | `external/tests/nrntest/nrniv/ida/*.ses` (gitignored); `~/models/nrndc1sim` |
| **Plan A** forcing \(t^+\) for free \(y'\) | **A0–A5 done** | continuous `Vector.play` + LM `dforce` / FD |
| **MOD density `PROCEDURE dforce`** | **Parked** | tip `210c43c56` on `hines-grok/ida-mod-dforce-park` only |
| **(b) Source-current discontinuities** | **Done (v1+polish)** | Plan: `~/neuron/notes/ida_plan_b_source_currents.md`. PWLClamp; E0–E4; end free-\(y\); electrode–xc seed. **Docs:** `dae_init_mode` electrode/xtral notes in `cvode.rst`. **E2:** true Vm is `seg.v` (held); `vext` may jump when `xc=0`. **CI:** `ctest -R hoctests::test_ida_source_current`. Smoke `nrndc1sim`: mode-3 OK at `finitialize`; per-step `re_init` after transfer often falls back (algebraic clamp/current). Default still mode 0. |

**Working tip:** `7688c6238` — *A5: IDA IC path stats, clearer mode-3 fallback messages*  
**Branch:** `hines-grok/ida-init` (tracks `origin/hines-grok/ida-init`)  
**Default remains mode 0.** Mode 3 ready for broader validation; residual fail → heuristic (except audit armed).

### Parked: density MOD `dforce`

| Item | Value |
|------|--------|
| Branch | `hines-grok/ida-mod-dforce-park` (also on origin) |
| Commit | `210c43c56` *Call density PROCEDURE dforce at IDA IC* |
| Content | `src/nrnoc/mod_dforce.cpp`, `test/hoctests/mod_dforce.mod`, hook in `nrndaspk.cpp`, docs, test |
| On `ida-init` | **Not present** (reset to A5 after park) |
| Intent | Variable-capacitance / assigned-rate MOD hook; **do not revive** until param-in-C / charge work is scheduled |
| Restore | `git cherry-pick 210c43c56` or merge that branch |

Motivation for park: that commit was a rush on **time-dependent capacitance** rates. Preferred next slice is **source currents**, not density `dforce`. Defer param-in-C and charge-conservation patterns (`dcmdt` / `NET_RECEIVE` charge jumps).

### Plan A (forcing \(t^+\) info) — done

**Forcing \(t^+\) info:** right-limit value \(u(t^+)\) and classical derivative \(u'(t^+)\) of exogenous drives after a discontinuity (or at `finitialize`). Geometric / DAE literature: **1-jet** of \(u\) at \(t^+\).

| Slice | Status | Notes |
|-------|--------|--------|
| **A0** Spec + continuous-play \(t^+\) oracle | **Done** | `src/nrniv/vecplay_tplus.h`; `VecPlayContinuous::forcing_tplus` |
| **A1** Wire play instances at IC | **Done** | `nrn_collect_forcing_tplus` at `Daspk::init`; `Daspk::last_forcing_tplus` |
| **A2** Mode 3 free \(y'\) from \(u'\) (LM / series CR) | **Done** | null(C) via \(Z^\top G y' = Z^\top b'\); ramp \(V_1'=1,V_2'=0.5\) |
| **A3** Multi-event + finitialize suite | **Done** | istep, kink, end extrap, flat end, finitialize slope, multi-event |
| **A4** LM `dforce` thin API | **Done** | `LinearMechanism.dforce(callable, bdot)`; FD fallback |
| **A5** Polish / diagnostics | **Done** | `dae_init_stats`; clearer fallback msgs; counters |

Continuous play \(t^+\) rules: hold for \(t < t_0\); **outgoing** slope at a knot (including \(t=t_0\)); **linear extrapolation of the last two points** past the end (flat last segment ⇒ \(u'=0\)).

---

## Why this worktree

Focused DAE/IDA initial-condition work. Branch from **master**; do not revive permanent VMX / `v12` state doubling from historical `origin/idainit`.

| Worktree / path | Branch | Purpose |
|-----------------|--------|---------|
| `~/neuron/nrnida` | `hines-grok/ida-init` | IDA IC modes, battery project, audit, Plan A |
| park | `hines-grok/ida-mod-dforce-park` | Density MOD dforce (local/remote park) |
| notes | `~/neuron/notes/` | Design notes outside the repo |

---

## North star (success criteria)

After `finitialize` and after each discontinuity (`NET_RECEIVE`, `Vector.play`, `at_time`, clamps, …):

1. **Residual:** \(g(y', y, t) = 0\) within integrator WRMS / ewt tolerance.
2. **Physical \(dt\to 0\):** hold continuous content; free absolute algebraics as network requires.
   - **Hold:** ODE states; capacitor \(\Delta v\) / charge; inductor current; membrane \(V_m\) with extracellular; \(xc>0\) layer drops.
   - **Free to jump:** absolute node voltages (e.g. series \(C\)–\(R\) + \(I\) step).

Nano-\(dt\) fully implicit Euler is a **fallback**, not the definition of (2). Mode 3 recovers \(y'\) from \(C y' = f(y,t)\) at fixed projected \(y\).

### Non-goals (still)

- Permanent `USE_VMX` / doubled continuous state for integration (`origin/idainit`).
- Block on SUNDIALS 3 / PR #1960 (`slds`) for IC design (revalidate later).
- Pure speculative \(g_{\mathrm{ic}}\) stacks without industry map (Phase R already done).
- **Now deferred:** density `PROCEDURE dforce`; param-in-C; charge-conservation / `dcmdt` productization.

---

## Architecture (current)

```text
Discontinuity / finitialize
    │
    ▼
Daspk::init()  [src/nrncvode/nrndaspk.cpp]
    │  collect continuous Vector.play forcing t+ (A1)
    │  mode 0 → heuristic nano-step (dae_init_dteps)
    │  mode 1 → IDACalcIC(IDA_Y_INIT) then heuristic fallback
    │  mode 2 → IDA_Y_INIT only
    │  mode 3 → battery y-project → y' from C*y'=f(y)
    │            + null(C) free y' from play b' / LM.dforce (A2–A4)
    │            residual fail → heuristic
    │            (audit armed: no fallback; diagnose top residual eqs)
    ▼
Cvode::res  →  G ≈ C y' − f(y)  (membrane cm, xc layers, nrndae_dkres)
```

| Piece | Location |
|-------|----------|
| IC modes / audit / \(C y'=f\) / free \(y'\) | `src/nrncvode/nrndaspk.{h,cpp}` |
| HOC API | `src/nrncvode/cvodeobj.cpp` (`dae_init_mode`, `dae_init_dteps`, `dae_init_audit`, `dae_init_audit_file`, `dae_init_stats`) |
| Battery project (LM) | `src/nrniv/linmod.cpp` `battery_ic_project()` |
| LM `dforce` / bdot | `src/nrniv/linmod1.cpp`, model side in linmod |
| Continuous play \(t^+\) | `src/nrniv/vecplay_tplus.h`, `vrecitem` |
| NrnDAE entry + LM \(y'\) seed | `src/nrniv/nrndae.cpp` |
| Extracellular battery | `src/nrnoc/extcelln.cpp` `nrn_extracellular_battery_ic()` |
| Soft factor fail | `src/nrnoc/solve.cpp` + `nrn_sparse13_soft_fail` |
| Built-in IClamp | `src/nrnoc/stim.mod` (`ELECTRODE_CURRENT i`, `at_time(del)`, `at_time(del+dur)`) |
| Docs | `docs/progref/simctrl/cvode.rst` |
| Tests | `test/hoctests/tests/test_ida_init_mode.py` |
| Design notes | `~/neuron/notes/ida_y_init_adoption.md`, `ida_phase_R_industry_map.md` |

### Mode 3 idea (productized)

1. **Project \(y\):** hold continuous content (LM floating \(C\), diagonal \(L\), op-amp lag; extracellular \(V_m\) / \(xc>0\) drops); free absolute algebraics as the network requires.
2. **Recover \(y'\):** diagonal / simple-mass solve \(C y' = f(y,t)\) (membrane \(10^{-3} c_m\), mechanism identity, LM single-column/difference stamps). **No `dteps`.**
3. **Free algebraic rates:** when continuous play (or LM force) has classical \(u'\), adjust \(y'\) in null(\(C\)) via \(Z^\top G y' = Z^\top b'\).
4. **Residual check:** on failure, classify top residual eqs; fall back to nano-step unless audit is armed.

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

## Next: (b) Source-current discontinuities

### Scope (preferred)

Focus on **electrode / source current** discontinuities and their classical \(t^+\) rates, **not** variable \(C\) / charge-conserving MOD capacitance.

| In scope | Out of scope (defer) |
|----------|----------------------|
| `IClamp` step (`at_time` + piecewise constant \(i\)) | Density `PROCEDURE dforce` (parked) |
| Sinusoid / continuous \(i(t)\) with kinks or known \(i'\) | Param-in-\(C\) / time-dependent `cm` productization |
| Interaction with **extracellular** (electrode current → `vext` network) | `dcmdt` / charge-jump PP patterns productization |
| How \(i\) and \(i'\) enter residual / free \(y'\) | Thread B (`vext=0` INITIAL abandonment) as primary |

### Framing already accepted (discussion)

1. **Built-in source pattern (NMODL)** is already the right shape for steps:
   - `ELECTRODE_CURRENT i` (not transmembrane — matters for extracellular).
   - `at_time(t_event)` so DASPK reinit sees breakpoints.
   - Piecewise `BREAKPOINT` assignment of \(i\) (e.g. `stim.mod`).
2. **Jump in \(i\)** (step on/off): primarily a **\(y\) algebraic** problem after hold — free voltages jump; continuous content held. Residual \(G \approx C y' - f\) absorbs the new \(i\) into \(f\) once \(y\) is on the manifold; classical \(i'\) is zero almost everywhere between steps.
3. **Classical \(i'\)** matters when free algebraic \(y'\) must track a **continuous** source with nonzero derivative (ramps, sinusoids, continuous play into a force term) — same role as Plan A \(b'\) for LM: only the **null(\(C\))** free directions need \(i'\) / \(b'\).
4. **Extracellular** is “harder \(y\)”, not new keywords: electrode current couples into the extracellular network; battery IC already holds \(V_m\) / \(xc>0\) drops. Source (b) should validate that path under IClamp ± `extracellular`, not invent new NMODL.
5. **Do not** treat density MOD `dforce` as the vehicle for (b). That park was for assigned **capacitance rates**. Source work should reuse **play \(t^+\)**, **LM.dforce**, and/or **explicit electrode \(i,i'\) policy** as appropriate.

### Suggested order of attack (when coding starts)

Prefer **discussion → minimal spike → tests** over large productization.

1. **Catalog** how `IClamp` / `at_time` already force reinit; what residual looks like mode 0 vs 3 on a pure istep into a compartment (with/without LM series \(C\)–\(R\)).
2. **Spike:** IClamp step at known \(t\) under mode 3 — residual + physical \(\Delta v\) hold; compare audit panels A/B/C.
3. **Spike:** continuous sinusoid (or play into amp) where \(i'\) is known — does free \(y'\) need an electrode analogue of A2?
4. **Spike:** same + `extracellular` — battery hold vs free `vext` nodes.
5. Only then: thin API if something is missing (do **not** default to density `dforce`).

### Relation to residual vs “charge conservation”

- Residual success: \(g(y',y,t)\approx 0\) after IC.
- Physical success: capacitor content continuous; absolute nodes free to jump when \(I\) steps.
- Charge-conserving **parameter jumps in \(C\)** are a **different** problem (parked with density dforce / dcmdt). Do not conflate with electrode \(i\) steps.

---

## Other deferred / optional work

1. Broader model validation; event-reinit matrix of models.
2. Thread **B:** abandon fixed `vext=0` INITIAL when needed (e.g. forced `e_extracellular`).
3. Policy: when to recommend mode 3 over 0; `cm→0` / ideal clamp as algebraic.
4. SUNDIALS 3 revalidation; do **not** resurrect permanent VMX.
5. Untracked local `docs/index.rst` in worktree — leave alone unless docs build work.

---

## Build

Typical local build: `~/neuron/nrnida/build/`.

```bash
cd ~/neuron/nrnida
mkdir -p build && cd build
cmake .. -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DNRN_ENABLE_TESTS=ON \
  -DNRN_ENABLE_MPI=OFF
ninja -j$(nproc)
```

**Note:** build tree may still contain objects from the parked `mod_dforce` commit (e.g. `mod_dforce.cpp.o`). After switching to A5 tip, a clean rebuild is safer if linking oddities appear: `ninja -t clean` then `ninja`, or reconfigure.

---

## Tests

```bash
cd build
export PYTHONPATH="$PWD/lib/python${PYTHONPATH:+:$PYTHONPATH}"
export LD_LIBRARY_PATH="$PWD/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# Prefer install tree or PATH that matches the built nrnivmodl if tests compile mod:
export PATH="$PWD/bin:$PATH"

python ../test/hoctests/tests/test_ida_init_mode.py
```

Manual: `~/models/nrndc1sim` with `h.cvode.dae_init_mode(3)` before `GUI()` / run.

---

## Starting prompt (new session)

```
Read ~/neuron/nrnida/GROK-IDA-INIT.md and, if present,
~/neuron/notes/ida_y_init_adoption.md and ida_phase_R_industry_map.md.

Repo: ~/neuron/nrnida, branch hines-grok/ida-init @ 7688c6238 (A5).
Parked density dforce: hines-grok/ida-mod-dforce-park @ 210c43c56 — do not revive by default.
Build: prefer existing build/; clean rebuild if parked objects linger.

Goal: consistent IDA IC after finitialize and discontinuities —
residual ~0 AND physical dt→0 (hold continuous content).

Done: dae_init_mode 0–3; mode 3 = battery y-hold + y' from C*y'=f + Plan A
forcing t+ (play + LM.dforce) for free y'; audit; A5 stats.

Next (b): source-current discontinuities (IClamp / sinusoid + at_time,
± extracellular). Discussion-first. Defer param-in-C, charge conservation,
density PROCEDURE dforce. Reuse Plan A framing: i jumps → free y; i' only
for free algebraic y' when classical derivative is nonzero.

Do not revive permanent VMX/v12. Default stays mode 0 until mode 3 is
broadly validated.
```

---

## Historical context

| Branch / path | Lesson |
|---------------|--------|
| `origin/idainit` | `IDA_YA_YDP_INIT` + permanent v12/VMX; LinearCircuit better; extracellular never fully worked; doubled state |
| Pure `IDA_Y_INIT` (mode 1/2) | Algebraic LM OK; folded caps → singular \(\partial g/\partial y\) at \(c_j=0\) |
| Nano-step heuristic | Makes \(\|g\|\) small by finite \(C/\mathrm{d}t\); \(O(\mathrm{dteps}\cdot\|y'\|)\) residual bias (fails tiny \(c_m\)) |
| Mode 3 \(C y'=f\) | Clears residual when \(y\) already on manifold (SEClamp + tiny \(c_m\) scm2eem) |
| Density MOD dforce (`210c43c56`) | Parked; wrong priority vs source currents; capacitance-rate hook |

Industry map (Phase R): hold differentials / content; solve algebraics; recover \(y'\) from mass equation.
