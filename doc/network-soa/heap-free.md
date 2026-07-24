# L2b — Heap-free charter (`local/cpu-net-soa-heap-free`)

**Branch only — no PR** until a reviewable slice is green.  
**Base:** keep rebasing onto green `local/cpu-network-soa` (PR #3822) after master merges.

**Charter:** Follow CoreNEURON as much as feasible for the data plane and hot path.
Interpreter, structure change, and queue polymorphism stay NEURON-specific (policies below).

---

## Settled decisions

### Data plane (CoreNEURON-shaped)

| Item | Choice |
|------|--------|
| Weight block | One logical block of length `arity` per NetCon edge |
| NetCon shell / SoA | **O(1)** — no `vector` of per-arg owning handles |
| Record of base | NetCon SoA `(WeightIndex, WeightCount)`; arity also known from target type |
| Weight storage | Flat / Weight SoA pool; deliver uses base index |
| Indices in bulk tables | Prefer indices (`int` / `uint32_t` typedefs), not `NetCon*` / `size_t`, for weights and fanout |
| Fanout | Source-centric range (`NcIndex`/`NcCount`) into ordered NetCons |
| Packing | **A** — physical layout; thread packing required; target-instance adjacency in the same sort key (FOR_NETCONS-friendly) |
| SelfEvent identity | `weight_index` (`-1` if no NetCon); never temp `double*` as identity |
| FOR_NETCONS | Index walk over contiguous bases under A (nocmodl codegen change as needed) |
| `NET_RECEIVE` INITIAL | Index-shaped API preferred; finitialize-only |

### NEURON-only policy

| Item | Choice |
|------|--------|
| HOC `NetCon.weight[i]` | SoA `data_handle` from base+i; `Object*` shell not rewritten on permute |
| Ownership | NetCon owns the weight **block**; destroy invalidates the block (reclaim now or at compact/sort) |
| Target PP destroyed | Null `target_`, deactivate; **weights stay** until NetCon destroyed |
| Teardown order | `pc.gid_clear` → destroy NetCons → destroy cells (recommended) |
| Structure change / NetCon free with live network queue | **Clear queue** (or later generation-noop); no mid-run surgery requirement |
| Freeze | After `finitialize`, connectivity/layout fixed until next `finitialize` (or explicit clear + re-init) |
| TQueue | Keep `(tdeliver, DiscreteEvent*)` for now; compress later if needed |
| GPU | Out of scope on this branch |

### Explicitly not required for v1

- Arbitrary model edits between `pc.psolve()` with a live queue  
- Queue scrub or refcount-pin on every NetCon destroy  
- Full nocmodl ABI change to `pnt_receive(…, weight_index, flag)` before materialize shim is removable  
- Permanent FOR_NETCONS perm table **if** sort key already groups by target instance (**A**)

---

## Contract (identity)

> A NetCon weight block is identified by a base row in the Weight SoA (arity fixed by target mechanism type). Queued SelfEvents and generated `net_send` carry that base as `int` (or −1). FOR_NETCONS never walks foreign `double*`; it walks a target-local list of bases, contiguous after sort under packing **A**. Per-NetCon `weight_` heap is MOD scratch only until codegen/tests allow deletion.

---

## Implementation order (gateable)

1. **Docs + index typedefs** — this file; `weight_index_t` / `netcon_index_t`.  
2. **O(1) ownership path** — `WeightBlock` off-shell (`unique_ptr`); NetCon SoA base+count authority; HOC `weight_soa_handle(i)`.  
3. **Fanout tables as indices** — `g_network_fanout_order` holds `netcon_index_t` (SoA rows); resolve via `g_netcon_by_soa_row`.  

4. **nocmodl FOR_NETCONS** — walk Weight SoA bases (`_nrn_netcon_weight_bases` /
   `_nrn_fornetcon_weight`); sort packs by target instance (packing A).  

5. **Ephemeral MOD scratch only** — thread-local buffer for non-FOR_NETCONS
   `pnt_receive`; TLS active `weight_index` so `net_send(..., _w)` never queues
   scratch as identity.  
6. **Delete `NetCon::weight_`** — SoA only; FOR_NETCONS owns per-target scratch
   buffers keyed by weight bases; `weight_soa_data()` for contiguous double* APIs.  
7. Rebase onto green PR tip after each PR↔master refresh.

---

## Workflow

```text
master ──► local/cpu-network-soa (PR #3822, keep green)
                └── local/cpu-net-soa-heap-free (this branch, private)
```

Prefer rebase of heap-free onto PR tip after small green PR fixes; merge if history is too messy to rewrite.

---

## Related

- Topology / CoreNEURON map: [topology.md](topology.md)  
- Dual-write history: [dual-write-and-heap.md](dual-write-and-heap.md)  
- Sort: [sort-and-packing.md](sort-and-packing.md)  
- CoreNEURON: `src/coreneuron/sim/multicore.hpp`, `network/netcon.hpp`, `io/setup_fornetcon.cpp`
