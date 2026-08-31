# L2b — Heap-free charter (`local/cpu-net-soa-heap-free`)

**Branch only — no PR** until a reviewable slice is green.  
**Base:** keep rebasing onto green `local/cpu-network-soa` (PR #3822) after master merges.

---

## North star (two epochs)

**Sim epoch (CoreNEURON-shaped *plus* host improvements):**  
Flat/packed weights, integer bases, source fanout ranges, frozen connectivity for the run.
CoreNEURON is the **integration reference and hot-path shape**, not a performance ceiling
and not a product constraint on NEURON’s lifecycle.

Improvements CoreNEURON does not (or only partially) provide, that we intentionally pursue:

| Improvement | Notes |
|-------------|--------|
| **Weight physical packing (A)** | Sort by target thread → target PP instance → …; stronger locality than CN’s construction-order pool + FOR_NETCONS perm |
| **Dynamic `nthread` between runs** | Rebuild sort, thread offsets, fanout, FOR_NETCONS tables — CN setup is fixed |
| **HOC/Python edit between runs** | Handles survive permute; append/invalidate then re-sort |

**Edit epoch (full NEURON flexibility):**  
Create/destroy NetCons, reconnect, change `pc.nthread`, rebuild model. Not mid-`psolve`
with a live queue (structure change clears the queue).

One line:

> **Build and edit like NEURON; once the run starts, look like CoreNEURON’s network data plane
> (with better host packing and reconfigurable threads)—then tear down and edit again.**

**Charter (implementation):** sim path stays index- and SoA-first; interpreter, structure change,
and queue polymorphism stay NEURON-specific (policies below).

---

## Settled decisions

### Data plane (CoreNEURON-shaped + packing A)

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

### Explicitly not required for heap-free v1 (steps 1–6)

- Arbitrary model edits between `pc.psolve()` with a live queue  
- Queue scrub or refcount-pin on every NetCon destroy  
- Permanent FOR_NETCONS perm table **if** sort key already groups by target instance (**A**)

---

## Contract (identity)

> A NetCon weight block is identified by a base row in the Weight SoA (arity fixed by target mechanism type). Queued SelfEvents and generated `net_send` carry that base as `int` (or −1). `NET_RECEIVE` / FOR_NETCONS resolve MOD `double*` via contiguous SoA zero-copy, or TLS materialize when scattered. There is **no** per-NetCon weight heap and **no** `SelfEvent::weight_` pointer.

---

## Implementation order (gateable)

### Done on this branch (steps 1–6)

1. **Docs + index typedefs** — `weight_index_t` / `netcon_index_t`.  
2. **O(1) ownership path** — `WeightBlock` off-shell; SoA base+count authority; HOC handles.  
3. **Fanout tables as indices** — `g_network_fanout_order` = SoA rows; `g_netcon_by_soa_row` resolve.  
4. **nocmodl FOR_NETCONS by bases** — `_nrn_netcon_weight_bases` / `_nrn_fornetcon_weight`; packing A sort key.  
5. **Ephemeral MOD scratch** — TLS for non-FOR_NETCONS; active `weight_index` so scratch is never queue identity.  
6. **Delete `NetCon::weight_`** — SoA only; FOR_NETCONS per-target scratch; `weight_soa_data()` for legacy double* APIs.

### Process

7. Rebase onto green PR tip after each PR↔master refresh.

### Next phase — sim path closer to index-native CoreNEURON, still NEURON edit epoch

| Substep | Scope | Goal |
|---------|--------|------|
| **6b — Zero-copy host receive** | **Done:** contiguous block → `pnt_receive` via `data_if_contiguous()` / `weight_soa_data()`; TLS materialize only if scattered | Drop copy when packed; identity stays `weight_index` |
| **7a — nocmodl index ABI** | **Done:** `pnt_receive_t(Point_process*, int weight_index, double flag)`; generated body keeps `_args[i]` via `_nrn_netrec_wsoa` / `_done`; `net_send` / `artcell_net_send` take index (−1 if none) | CN-shaped interface; identity is index only |
| **7b — FOR_NETCONS without owned double pool** | **Done:** `ForNetConsInfo` = bases only; `_nrn_fornetcon_weight` zero-copy SoA or shared TLS view; no `weight_storage` / `argslist` / base→buf map | Drop per-target peer double pool |
| **7c — Cleanup** | **Done:** drop `SelfEvent::weight_`, drop `_nrn_netcon_args`; TLS only for scattered blocks (zero-copy when packed); commit only when materialize was used | Host path SoA/index; no dead double* identity |

Optional later: GPU upload of the same packed columns (sibling track), not a prerequisite for 7.

### Host path shape after 7a–7c

| Item | State |
|------|--------|
| NetCon weight storage | Weight SoA only (`WeightBlock`) |
| SelfEvent / net_send identity | `int weight_index` (−1 if none) |
| `pnt_receive` / INITIAL | `(Point_process*, int, double)` |
| Primary edge `_args[i]` | Contiguous SoA zero-copy, else TLS + commit |
| FOR_NETCONS | Bases list; per-edge SoA/TLS (no owned peer pool) |
| TLS | Kept as **scattered-block fallback** only (packing A + sort → usually zero-copy) |

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
