# Network SoA — developer docs (map)

**Audience:** people working on `local/cpu-network-soa`  
**Status tracker / session handoff:** `GROK-NETWORK-SOA.md` (repo root)  
**Phase-0 field tags & checklist:** `doc/network-soa-phase0.md` (detailed, longer)

Docs here are **segregated by height**. Prefer the shortest document that answers your question.

| Level | Doc | Length / purpose |
|-------|-----|------------------|
| **L0 — North star** | this file §North star | One screen: goals, non-goals, current posture |
| **L1 — Topology** | [topology.md](topology.md) | PreSyn → NetCon → Point_process; CoreNEURON mapping; fanout authority; rank/thread placement |
| **L2 — Dual-write & heap** | [dual-write-and-heap.md](dual-write-and-heap.md) | What is primary where; why `weight_` still exists; FOR_NETCONS / INITIAL / SaveState |
| **L3 — Sort & packing** | [sort-and-packing.md](sort-and-packing.md) | What `nrn_ensure_model_data_are_sorted` does to network containers |
| **L4 — Spec detail** | `../network-soa-phase0.md` | Field tags, sidecars, SaveState notes, open questions resolved |
| **Ops** | `../../GROK-NETWORK-SOA.md` | Build/test gates, commit status, “what next” |

---

## North star (L0)

**Goal:** One CPU backing store for *integration-hot* network data (columns + indices), with HOC wrappers as permutation-stable handles — same *spirit* as node/mechanism SoA and CoreNEURON’s per-thread flat arrays.

**Integration reference:** CoreNEURON (`NrnThread::{pntprocs,netcons,weights,presyns}`, `PreSyn::{nc_index_,nc_cnt_}`, `NetCon::u.weight_index_`).

**Not the first PR’s job (unless we change strategy):**

- Full free of `NetCon::weight_` under default **nocmodl** ABI  
- GPU net buffers (sibling track after SoA is usable)  
- Thin remote `InputPreSyn` / gid→fanout table  

**Current posture (2026-07):** Dual-write complete for CoreNEURON-shaped columns; network participates in global sort; SaveState/BBSaveState dual-write; **`weight_` remains long-lived MOD scratch** because generated `pnt_receive` / `net_send(..., _w)` still use `double*`.

**PR vs master:** A PR can document progress and gather review without claiming “land now.” Landing criteria (performance/space *and* preferably a path to GPU) are product decisions separate from “is the dual-write design sound?”

---

## Two graphs (memorize this)

```text
HOT PATH (spike → delivery) — CoreNEURON shape, indices:
  PreSyn ──(NcIndex,NcCount)──► ordered NetCon fanout
      NetCon ──Target──► PointProcess row
      NetCon ──WeightIndex──► contiguous weight block

MANAGEMENT / HOC (interactive NEURON) — pointers/handles still exist in dual-write:
  PreSyn.dil_ / NetCon.src_ / NetCon.target_ / Object*
```

**Fanout authority** = which of the two is allowed to decide “who receives this spike” on the hot path. See [topology.md](topology.md).

---

## Related code

| Concern | Location |
|---------|----------|
| Containers | `src/neuron/container/network/*.hpp` |
| Sort / repack | `src/nrncvode/network_soa_sort.cpp` |
| Fanout dual-write | `PreSyn::ensure_fanout_order` in `netcvode.cpp` |
| Receive by index | `nrn_pnt_receive_by_weight_index` |
| CoreNEURON layouts | `src/coreneuron/sim/multicore.hpp`, `network/netcon.hpp` |
