# Grok handoff: NEURON + NMODL on CPU (nocmodl replacement path)

Use this file when starting a **new** Grok session rooted in
`~/neuron/nrnnmodl`.

**Not** the GPU-native tree. GPU / Traub-scale threshold (Th4) is **paused** in
`~/neuron/nrngpu` until this project can build and validate Traub mechanisms
under NEURON via NMODL on **CPU**.

| Sibling tree | Role |
|--------------|------|
| `~/neuron/nrnnmodl` | **This project** — NEURON host + NMODL codegen for model mods |
| `~/neuron/cpu_net_soa` | Heap-free network SoA platform (`GROK-NETWORK-SOA.md`); **base of this branch** |
| `~/neuron/nrngpu` | Native GPU on heap-free (`GROK-GPU-NATIVE.md`); Th4 paused until M1–M2 here |
| `~/models/82894` | Traub ModelDB 82894 (shared benchmark / prcellstate) |

---

## Canonical repo and branch

| Item | Value |
|------|--------|
| Repo | `~/neuron/nrnnmodl` (clone of `neuronsimulator/nrn`) |
| Branch | `local/nmodl-cpu-traub` |
| Base | **`local/cpu-net-soa-heap-free`** (not master — weight/NET_RECEIVE ABI differs) |
| Build | CPU-first install under e.g. `~/neuron/nrnnmodl/build` (no GPU requirement for M0–M2) |

**Why not master:** heap-free changes `pnt_receive` to a `weight_index` ABI and updates both NMODL and nocmodl. Master-built mechs will not correctly integrate with heap-free / `nrngpu` consumers.

Wire into local env when ready, e.g. `nrnenv` entry for `nrnnmodl` → this install.
Until then, activate via `cmake --install` path + `PATH` / `PYTHONPATH` as usual.

---

## Goal

**NEURON fixed-step CPU** runs Traub (and, by extension, general model MOD files)
using **NMODL** to generate NEURON mechanism code — on the path to **NMODL
replacing nocmodl** for NEURON.

Primary acceptance (incremental):

1. **Build:** `nrnivmodl` with NMODL produces a loadable Traub `libnrnmech` /
   `special` against this NEURON.
2. **Correctness:** CPU prcellstate / spikes match **NEURON + nocmodl** on the
   same model knobs (see oracles below).

Out of scope for **this** project until M2 is green:

- Native GPU fixed-step / Gate B–C / Th4 threshold load
- `NRN_GPU_ALLOW_UNQUALIFIED` Traub mixed mode
- Performance vs CoreNEURON GPU

---

## Why this exists (context from nrngpu, 2026-07-25)

Attempted Traub on `local/gpu-native-net-soa` after ringtest Th0–Th3 green:

1. **NMODL + OpenACC** `nrnivmodl` for Traub failed at compile:
   generated code `#include "neuron/model_data.hpp"` but the **install tree**
   only exposed `neuron/model_data_fwd.hpp` (source has
   `src/neuron/model_data.hpp`). Catastrophic nvc++ error on first Traub mods.
2. **Fallback NOCMODL** Traub linked to `nrngpu` and ran **CPU** fine.
3. **GPU** Traub: `QUALIFIED: no` (Gate B/C — all Traub mechs host
   CURRENT/JACOBIAN/SOLVE). With `NRN_GPU_ALLOW_UNQUALIFIED=1`, first step
   **segfaulted** in host `_nrn_cur__AMPA` inside `nrn_rhs` /
   `fixed_step_thread` — no prcellstate dump.
4. Gate **E** (threshold) was already yes; the blocker was mechanism
   build/ownership, not PreSyn detect.

Conclusion (agreed): **pause Th4 / Traub GPU**. Stand up NEURON+NMODL **CPU**
first; use existing oracles; resume GPU Traub only after NMODL mechs are real
under NEURON.

---

## Oracles (do not invent new truth)

| Oracle | Role |
|--------|------|
| **NEURON + nocmodl** | Primary host truth for Traub dynamics / prcellstate / spikes |
| **CoreNEURON + NMODL CPU** | NMODL semantics + packing without NEURON step driver |
| **CoreNEURON + NMODL GPU** | Later perf/reference only — **not** first NEURON NMODL bar |

Compare NEURON+NMODL CPU to **nocmodl CPU** first. If they disagree, use
CoreNEURON NMODL CPU to see whether the bug is NEURON integration vs NMODL
codegen.

---

## Traub harness (shared model)

| Item | Path / knobs |
|------|----------------|
| Model | `~/models/82894` |
| One-tenth network | `one_tenth_ncell=1`, `use_gap=0`, `nthread=1` |
| Focus cell | `prcellstate_gid=171` |
| Helper | `./hinesrun.sh <mytstop> <checkpoint>` (CPU then GPU in that script — use **CPU-only** invocations here) |
| Compare | `python3 rdcellstate.py --ignore-unused --ignore-ion …` |
| Docs in model | `BENCHMARK.md`, `init.hoc`, older notes `~/neuron/notes/native_gpu_traub_parity.md` (GPU history; CPU knobs still useful) |

Example CPU-only one step (after `special` built with **this** NEURON + NMODL):

```bash
cd ~/models/82894
# Point special at nrnnmodl install (rebuild x86_64 after switching NEURON)
./x86_64/special \
  -c one_tenth_ncell=1 -c use_gap=0 -c nthread=1 \
  -c benchmark_quiet=1 -c enable_gpu=0 \
  -c prcellstate_gid=171 -c mytstop=0.025 -c prcellstate_checkpoint=-1 \
  init.hoc
```

Spike-scale reference (nocmodl / benchmark config): ~**4474** spikes @ 100 ms
for the standard 1/10 no-gap setup (confirm against your oracle run; do not
assume GPU counts).

---

## Milestones

| ID | Content | Done when |
|----|---------|-----------|
| **M0** | NMODL → NEURON mech **build** for Traub (CPU toolchain; no GPU req) | `nrnivmodl` with NMODL produces `special` / `libnrnmech`; loads under this install |
| **M1** | One-step parity | gid 171, `tstop=0.025`, prcellstate vs nocmodl CPU (noise-level policy as agreed) |
| **M2** | Multi-step + spikes | Short multi-step prcellstate; then spike parity at agreed `tstop` (e.g. 100 ms) vs nocmodl |
| **M3** | Gap inventory | Document remaining NEURON-only NMODL holes (WATCH, artcell, VERBATIM, …) vs nocmodl |
| **Later** | Resume `nrngpu` Traub / Th4 | Only after M1–M2; OpenACC/Gate B–C is a **consumer** of M0+ |

### M0 suspected first fix areas

- Install / public headers: ship or redirect so NMODL-generated code can
  `#include "neuron/model_data.hpp"` (or change codegen to the correct public
  header).
- `nrnivmodl` / CMake: NMODL flags for **NEURON host** (not only CoreNEURON
  ACC), include paths, link against this `libnrniv`.
- Confirm registration path matches modern SoA (`MechanismRange`, etc.) the
  same way in-tree built-ins do.

Do **not** start with OpenACC Traub. CPU NMODL first.

---

## Relationship to NMODL replacing nocmodl

- **Near term:** Traub is the forced stress test (many channels, synapses,
  ions, point processes).
- **Medium term:** default `nrnivmodl` can prefer NMODL for NEURON mechs when
  parity gates pass.
- **Long term:** nocmodl remains fallback until M3 gaps are closed or
  explicitly deprecated.

CoreNEURON already lives on NMODL; this project is the **NEURON host** half.

---

## Agent rules (this tree)

1. Work and commit in **`~/neuron/nrnnmodl`** only (unless user points elsewhere).
2. **CPU NMODL first** — no GPU Traub rabbit holes; no Th4.
3. Prefer **execute** (configure, build, `nrnivmodl`, short prcellstate) over
   hand-waving.
4. Keep commits local until user asks to push; end milestone steps with a
   commit when asked (same convention as `nrngpu`).
5. Do not reintroduce heap `NetCon::weight_` work here — that lives on network
   SoA / GPU branches.
6. If a change belongs only on GPU-native, note it and leave a breadcrumb in
   `~/neuron/nrngpu/GROK-GPU-NATIVE.md` rather than implementing it here.

---

## Starting prompt (paste into a new session)

```
Read GROK-NMODL-CPU.md (and AGENTS.md if present).

Tree: ~/neuron/nrnnmodl. Project: NEURON + NMODL on CPU so Traub (82894)
mechanisms build and match NEURON+nocmodl. GPU / Th4 paused in nrngpu.

Oracles: NEURON nocmodl (primary), CoreNEURON NMODL CPU (secondary).

Start at M0: make nrnivmodl+NMODL produce a loadable Traub special against
this install. Known issue from nrngpu attempt: generated code needs
neuron/model_data.hpp but install may only have model_data_fwd.hpp.

Do not work on native GPU Traub or threshold Th4 in this session unless
explicitly redirected.
```

---

## Status

| Date | Note |
|------|------|
| 2026-07-25 | Project chartered; initially on `master`. Handoff written. |
| 2026-07-25 | **M0 green (master scout).** NMODL Traub build/load OK; `nrnivmodl` auto `NMODL_PYLIB`/`NMODLHOME`. |
| 2026-07-25 | **M1 green on master (noise-level).** Same-install nocmodl vs NMODL one-step; V/matrix exact; tsave sentinels non-physics. |
| 2026-07-25 | **Rebased** onto `local/cpu-net-soa-heap-free` (`9eebc43d0`). **M0+M1 re-green:** NMODL Traub build/load with `weight_index` ABI; one-step vs same-install nocmodl — V/matrix/topology exact; mech max \|d\|≈1e-15 after ignoring `tsave`/`*_unused` (same story as master scout). |
| 2026-07-25 | **M2 full green.** Spikes exact t=100 (4474) nocmodl vs NMODL. Hang was O(N) `weight_index2netcon` on every NET_RECEIVE (nhost=1, nthread=1). Interim O(1) map; then **removed reverse map**: sim path uses CoreNEURON-style `weights[base]` via `weight_soa_ptr` in `_nrn_netrec_wsoa` / `_nrn_fornetcon_weight` (no NetCon*). |

### M0 findings (keep for next sessions)

1. **`model_data.hpp` is not the CPU NMODL blocker.**  
   NMODL `--neuron` codegen includes `neuron/cache/mechanism_range.hpp` (not `model_data.hpp`). The install-only-`model_data_fwd.hpp` issue was from the **nrngpu OpenACC / GPU** path. Leave full `model_data.hpp` install for later GPU consumers if needed; do not block M1 on it.

2. **Build recipe (this tree):**
   ```bash
   cmake -S . -B build -G Ninja \
     -DCMAKE_BUILD_TYPE=RelWithDebInfo \
     -DCMAKE_INSTALL_PREFIX=$PWD/build/install \
     -DNRN_ENABLE_NMODL=ON \
     -DNRN_ENABLE_CORENEURON=OFF \
     -DNRN_ENABLE_TESTS=OFF
   cmake --build build -j$(nproc) --target install
   export PATH=$PWD/build/install/bin:$PATH
   export PYTHONPATH=$PWD/build/install/lib/python
   export LD_LIBRARY_PATH=$PWD/build/install/lib
   ```

3. **`nrnivmodl` env for NMODL:** classic `nrnivmodl` now auto-exports `NMODL_PYLIB` (from configure-time `PYTHON_LIBRARY`) and `NMODLHOME` (install prefix). Without that, nmodl aborts (`NMODL_PYLIB not set` / `NMODLHOME not set`). Fix in `bin/nrnivmodl.in`.

4. **Traub build smoke (M0):**
   ```bash
   mkdir -p /tmp/traub-nrnnmodl-m0 && cd /tmp/traub-nrnnmodl-m0
   ln -sfn ~/models/82894/mod/*.mod .
   nrnivmodl .
   # special lists all 34 mechs; insert naf / new AMPA() / new GABAA() / new NMDA() work
   ```

5. **M1 run recipe (same NEURON install; only translator differs):**
   ```bash
   export PATH=~/neuron/nrnnmodl/build/install/bin:$PATH
   export PYTHONPATH=~/neuron/nrnnmodl/build/install/lib/python
   export LD_LIBRARY_PATH=~/neuron/nrnnmodl/build/install/lib
   # Oracle: wrap nocmodl to strip --neuron that NMODL-default nrnivmodl injects
   #   nrnivmodl -nmodl /tmp/traub-m1/nocmodl-wrap .
   # NMODL: nrnivmodl .
   # Run from ~/models/82894 with absolute special paths; dumps 171_gpu000_t0.025.nrndat
   python3 rdcellstate.py oracle.nrndat nmodl.nrndat --ignore-unused --ignore-ion
   # Treat tsave (−1e20 vs 0) as non-physics; V/matrix should match exactly at one step.
   ```

6. **M2 recipe / results (heap-free):**
   ```bash
   # specials: /tmp/traub-m1-heapfree/{oracle,nmodl}/x86_64/special
   # Multi-step prcellstate (gid 171):
   special -c one_tenth_ncell=1 -c use_gap=0 -c nthread=1 -c enable_gpu=0 \
     -c benchmark_quiet=1 -c prcellstate_gid=171 -c mytstop=1 init.hoc
   # Spikes (must leave prcellstate_gid=-1 so spike2file runs; writes out1.dat):
   special ... -c prcellstate_gid=-1 -c mytstop=2 init.hoc
   # Compare: sort out1.dat; exact (t,gid) multiset equality at tstop=2.
   ```
   - **Green:** multi-step pcs through t=1; spike identity through t=2 and **t=100 (4474)**.
   - **Hang root cause (fixed):** serial (`nhost=1`, `nthread=1`). Hot path recovered `NetCon*` from `weight_index` (O(N) then O(1) map). **CN-aligned fix:** drop reverse map; `_nrn_netrec_wsoa` / `_nrn_fornetcon_weight` use `SelfEventFields::weight_soa_ptr(base, count)` → `&model().weights()[base]`. NetCon→WeightIndex still maintained on allocate/sort (forward only).

7. **Next: M3** — document remaining NEURON-only NMODL gaps vs nocmodl (WATCH, artcell, VERBATIM, …).
