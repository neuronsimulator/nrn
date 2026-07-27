# Native GPU trajectory (Vector.record without full SoA)

**Branch:** `local/gpu-trajectory-native`  
**Status:** design frozen; implementation by phase commits  
**Depends on:** Gate F lastpart gating, device-owned V (deviceptr), device nonvint  

## Problem

Host `fixed_record_continuous` samples **host** SoA into `IvocVect`. On the
native-GPU path that forced a post-nonvint **full SoA pull** whenever
`Vector.record` was present (Gate F red for record). Integration state should
stay device-resident; recording only needs a few scalars per step.

## Non-goals (v1)

- CoreNEURON callback sandwich (`nrn2core_*` trajectory API as the runtime path)
- Per-double OpenACC `update self` gather (Core per-step inefficiency)
- Full SoA pull as the record mechanism
- Wall-clock GUI flush cadence (step chunks only)
- Zero-copy GPU write into live `IvocVect` host pages

## Design summary

```
device sources  →  device staging  →  D2H on flush  →  append IvocVect / plot
```

| Mode | When | Flush |
|------|------|--------|
| **Full-stretch** | Default when no live IV GraphLine | Once at end of psolve (or remaining steps) |
| **Chunked** | GraphLine / IV present, or env override | Every `C` steps + partial at end |

**Direction during psolve:** device → host only (trajectory sinks).  
**Gate F:** pure `Vector.record` covered by a complete plan does **not** require
full SoA. `AFTER_SOLVE` / `BEFORE_STEP` arts that need host SoA still do.

### Sink path (locked)

Always **device staging**, then host append on flush. Do not GPU-write directly
into `IvocVect` storage in v1 (avoids host-pointer present/dirty issues).

### Sample phase

Same ordering as NEURON host record / Core `nrncore2nrn_send_values`: after the
half-step state that `fixed_record_continuous` would have seen (after
BEFORE_STEP arts when present; aligned with lastpart). Exact hook lands in T2.

### Unsupported sources

Range pointers not resolvable to device SoA (or exotic records): **warn + fall
back to full SoA** for that psolve so interactive models do not die. Tighten to
abort under a debug env later if desired.

### Env (optional v1)

| Env | Meaning |
|-----|---------|
| `NRN_GPU_TRAJECTORY_CHUNK=N` | Force chunk size N (`1` ≈ per-step dense staging) |
| unset / `0` | Auto: full-stretch if no GraphLine; else default C (e.g. 50) |

## Phases (one commit group each)

| Phase | Deliverable | Smoke |
|-------|-------------|--------|
| **T0** | This doc + handoff | Read |
| **T1** | Host plan: walk `fixed_record_`, resolve sources/sinks | Unit / small check |
| **T2** | Staging + gather + full-stretch flush; Gate F unhook when plan complete | Ringtest ± record, 688@100 |
| **T3** | Chunked + env; GraphLine basics | Optional IV; ringtest regression |

## Implementation map (planned)

| Area | Path |
|------|------|
| Plan + staging + flush | `src/neuron/gpu/trajectory.{hpp,cpp}` (new) |
| Gate F consumer list | `src/neuron/gpu/lastpart.cpp` |
| Step hook | lastpart / fadvance fixed path (T2) |
| Request enumeration (reference) | `src/nrncvode/netcvode.cpp` `nrnthread_get_trajectory_requests` |
| Core gather reference | `src/coreneuron/sim/fadvance_core.cpp` `nrncore2nrn_send_values` |

## Reference only (not the runtime architecture)

CoreNEURON inserts a CPU layer between NEURON PlayRecords and GPU SoA
(type/index → Core gather → buffer or per-step scatter). Native GPU has one
SoA image: resolve `data_handle` / range id once to a **device pointer**, gather
into staging, D2H on flush. Reuse the **idea** (sparse gather, optional buffer),
not the callback sandwich.

## Constraints

1. Performance first; no unmeasured full SoA for record.  
2. Heap-free network unchanged.  
3. Ringtest long gate and Traub QUALIFIED stay green on shared paths.  
4. Commit per phase locally; do not push unless asked.
