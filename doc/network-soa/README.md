# Network SoA — developer docs (map)

**Audience:** `local/cpu-network-soa` (PR dual-write) and `local/cpu-net-soa-heap-free` (branch-only)  
**Status tracker / session handoff:** `GROK-NETWORK-SOA.md` (repo root)  
**Phase-0 field tags & checklist:** `doc/network-soa-phase0.md` (detailed, longer)

Docs here are **segregated by height**. Prefer the shortest document that answers your question.

| Level | Doc | Length / purpose |
|-------|-----|------------------|
| **L0 — North star** | this file §North star | One screen: goals, non-goals, current posture |
| **L1 — Topology** | [topology.md](topology.md) | PreSyn → NetCon → Point_process; CoreNEURON mapping; fanout authority; rank/thread placement |
| **L2 — Dual-write & heap** | [dual-write-and-heap.md](dual-write-and-heap.md) | Dual-write history; PR branch policy |
| **L2b — Heap-free charter** | [heap-free.md](heap-free.md) | Two-epoch north star; settled decisions; steps 1–6 done; next phase 6b–7 |
| **L3 — Sort & packing** | [sort-and-packing.md](sort-and-packing.md) | What `nrn_ensure_model_data_are_sorted` does to network containers |
| **L4 — Spec detail** | `../network-soa-phase0.md` | Field tags, sidecars, SaveState notes, open questions resolved |
| **Ops** | `../../GROK-NETWORK-SOA.md` | Build/test gates, commit status, “what next” |

---

## North star (L0)

**Goal:** One CPU backing store for *integration-hot* network data (columns + indices), with HOC wrappers as permutation-stable handles.

**Two epochs:**

| Epoch | Shape | Flexibility |
|-------|--------|-------------|
| **Sim** | CoreNEURON-shaped (indices, packed weights, frozen connectivity for the run) **plus** host improvements CN may not have (physical weight packing **A**, rebuild after `nthread` change) | No mid-run topology thrash |
| **Edit** | Full NEURON construction/modification between runs | Create/destroy, reconnect, `pc.nthread`, re-sort |

CoreNEURON is the **integration reference and hot-path shape**, not a performance ceiling. See [heap-free.md](heap-free.md) §North star.

**Integration reference:** CoreNEURON (`NrnThread::{pntprocs,netcons,weights,presyns}`, `PreSyn::{nc_index_,nc_cnt_}`, `NetCon::u.weight_index_`).

**Not the dual-write PR’s job:**

- GPU net buffers (sibling track after SoA is usable)  
- Thin remote `InputPreSyn` / gid→fanout table  
- nocmodl native `weight_index` receive (heap-free **next phase** 6b–7)

**Current posture (2026-07):** Dual-write on `local/cpu-network-soa` (PR #3822). Heap-free on **`local/cpu-net-soa-heap-free`**: steps 1–6 done (`NetCon::weight_` removed; TLS/FOR_NETCONS scratch remain as MOD bridges). Next: zero-copy SoA `double*` and/or nocmodl index ABI — [heap-free.md](heap-free.md).

**PR vs master:** Dual-write PR can gather review without claiming “land for perf.” Heap-free stays private until a green, reviewable slice exists.

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
