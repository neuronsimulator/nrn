# L3 — Sort and packing

Brevity: short–medium. Spec detail: phase0 §6, §8.

---

## When

`nrn_ensure_model_data_are_sorted()` after nodes + mechanisms. Network containers issue frozen tokens; if any unsorted → `sort_network_data`.

## Order of work

1. Sync dual-write shells → SoA (`ob2pntproc_0` for HOC PP; `soa_sync` NetCon/PreSyn).  
2. **PointProcess** — reverse-permute by `ThreadId`; set `cache.thread[i].point_process_offset`.  
3. **Weight** — repack so each NetCon’s `weight_soa_` rows are contiguous; NetCons ordered by (target thread, src PreSyn); set `weight_offset`.  
4. **NetCon** SoA — same order as weight packing; set `netcon_offset`; refresh indices.  
5. **PreSyn** — by thread; set `presyn_offset`; rebuild fanout (`NcIndex`/`NcCount` + order table).  

## Why this order

- Target-thread packing matches CoreNEURON **per-thread weight/netcon pools**.  
- Fanout rebuild **after** PreSyn/NetCon permutes so ranges match current rows.  
- Weight contiguity is required for `weight_index` base + `count` semantics (CoreNEURON).

## What sort does *not* claim

- Fanout physical layout ≡ NetCon SoA order (may use separate order array).  
- FOR_NETCONS adjacency (separate perm, CoreNEURON-style, still TODO for index path).  
- Freeing `weight_` heap.

## Implementation

`src/nrncvode/network_soa_sort.cpp`, header `neuron/container/network/sort.hpp`.
