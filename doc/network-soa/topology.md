# L1 — Topology: PreSyn → NetCon → Point_process

Brevity: medium. For field tags see `doc/network-soa-phase0.md` §5.

---

## 1. What “fanout authority” means

On a spike, NEURON must enumerate the NetCons that fire from a given source.

| Representation | Role historically | Authority? |
|----------------|-------------------|------------|
| `PreSyn::dil_` (`vector<NetCon*>`) | Interactive NEURON; append on connect; disconnect walks this | **Was** authority |
| `PreSyn` SoA `NcIndex`/`NcCount` + global order (`g_network_fanout_order` or, later, NetCon SoA range) | CoreNEURON-shaped fanout | **Should be** authority on hot path after dual-read |

**Fanout authority** = the structure the spike path *must* trust.  
Management code (connect, disconnect, HOC inspect) may still *write* `dil_` during dual-write, but must not leave delivery depending on a second, divergent truth.

Today (dual-write):

- Connect still fills `dil_`.
- `ensure_fanout_order()` rebuilds order + `NcIndex`/`NcCount` from `dil_` (and at sort / `init_events`).
- Spike path prefers SoA range when sorted, else falls back to `dil_`.

**Target end state:** rebuild ranges from connectivity at sort (or from a single edge table); `dil_` becomes optional sidecar or dies. Delivery never walks `dil_`.

That is the same *idea* as CoreNEURON: `nc_index_` / `nc_cnt_` into `netcon_in_presyn_order_[]`, not a per-PreSyn pointer vector.

---

## 2. CoreNEURON layout (what to keep in mind)

Per **thread** (after transfer), CoreNEURON holds flat arrays:

```text
nt.pntprocs[n_pntproc]     // Point_process { _type, _i_instance, _tid, ... }
nt.netcons[n_netcon]       // NetCon { active, delay, target_*, weight_index }
nt.weights[n_weight]       // flat double pool
nt.presyns[n_presyn]       // local/output PreSyn { threshold, gid, nc_index, nc_cnt, ... }
// plus InputPreSyn for remote gid→fanout only
```

### Spike fanout (source-centric)

```text
PreSyn.nc_index_ / nc_cnt_
    → netcon_in_presyn_order_[nc_index + i]   // NetCon* (setup), or index into nt.netcons
```

Source is **implicit**: NetCons in that range belong to this PreSyn. There is **no** hot-path `src_` on NetCon.

### Delivery (target-centric data, not fanout)

```text
NetCon.target_        → Point_process* into nt.pntprocs
NetCon.u.weight_index_ → base into nt.weights[]
pnt_receive(target, weight_index, flag)   // CoreNEURON: index, not double*
```

Weight **block length** is fixed by the target mechanism’s `NET_RECEIVE` arity (`pnt_receive_size[type]`). CoreNEURON does **not** store weight count on NetCon in the hot struct the same way; the size is known from `target_->_type`.

### FOR_NETCONS (target-centric *permutation*)

NetCons that share a target are **not** adjacent in construction order. CoreNEURON builds:

- `_fornetcon_weight_perm` — indices into `weights` so one target’s NetCon weight groups become adjacent in *index space*
- `_fornetcon_perm_indices` — displacement into that perm for each target instance  

So: **fanout order is by source PreSyn; FOR_NETCONS order is a separate target-grouped view of weight indices.** Two orderings, one weight pool.

### Rank / thread placement (your intuition)

| Constraint | CoreNEURON / multicore NEURON |
|------------|-------------------------------|
| NetCon target PP must be local to the rank that owns the synapse | Yes — remote connectivity is **gid → InputPreSyn fanout → local NetCons**, not a NetCon with a remote target pointer |
| NetCon delivery thread = target PP’s thread | Yes — `PreSyn::send` enqueues on `PP2NT(target)`; cross-thread only as interthread event |
| Weights live with **target** thread’s pool | Yes — `nt.weights` is per-thread (CoreNEURON) |

**NetCon is “about” the target PP** for:

- weight arity (`NET_RECEIVE` args)  
- delivery locality (same rank; same thread as target)  
- `pnt_receive` / FOR_NETCONS  

**NetCon is “about” the source PreSyn** only for:

- *who spikes it into existence* (fanout membership)  
- delay / active on that edge  

So both of these are true:

1. **Data plane packing:** prefer **target-thread** slices (weights, NetCon rows that deliver on that thread, PointProcess rows).  
2. **Spike enumeration:** **source-centric** ranges (`NcIndex`/`NcCount`).  

NEURON’s global SoA + **thread partition by permutation** (like nodes/mechs) is the elegant way to get (1) without per-thread heap graphs. Fanout ranges (2) are a *logical* order that may be a separate index array (CoreNEURON’s `netcon_in_presyn_order_`) even if NetCon SoA rows are packed by target thread.

---

## 3. How dual-write NEURON maps today

| CoreNEURON | NEURON dual-write |
|------------|-------------------|
| `nt.pntprocs[]` | `Model::point_processes()` + legacy `Point_process*` shell |
| `nt.netcons[]` | `Model::netcons()` + `NetCon` DiscreteEvent shell |
| `nt.weights[]` | `Model::weights()` + long-lived `NetCon::weight_` MOD scratch |
| `nt.presyns[]` | `Model::presyns()` + `PreSyn` shell + `dil_` |
| `nc_index_/nc_cnt_` | SoA fields + `g_network_fanout_order` (NetCon* table) |
| `weight_index_` | SoA `WeightIndex` (+ `weight_soa_.front().current_row()`) |
| `pnt_receive(..., weight_index, flag)` | `nrn_pnt_receive_by_weight_index` → still calls nocmodl `double*` form |

**Not yet CoreNEURON-true:**

- NetCon SoA row order is not required to equal fanout order (we have a **separate** fanout pointer table).  
- `pnt_receive` still takes `double*` (nocmodl).  
- `weight_` heap still allocated per NetCon.  
- Remote sources still fat PreSyn in `gid2in_` (InputPreSyn role deferred).

---

## 4. Ordering arrangements (summary table)

| Order | Key | Used for |
|-------|-----|----------|
| **Thread pack** | `thread_id` of target PP (weights/NetCons/PPs); PreSyn by its thread | Cache locality, future GPU upload slices, same as node/mech sort |
| **Spike fanout** | Source PreSyn (contiguous range) | `PreSyn::send` / deliver fanout |
| **FOR_NETCONS** | Target PP instance (perm of weight bases) | CoreNEURON `_fornetcon_*`; NEURON still walks heaps/pointers today |
| **HOC construction** | Object create order | SaveState lists, some BBSaveState assumptions — **not** hot-path locality |

Sort wiring today: partition PP/PreSyn by thread; pack weight blocks by NetCon ordered (target thread, src PreSyn); rebuild fanout ranges. That is deliberately **compatible** with CoreNEURON’s split between target-thread pools and source-centric fanout indices.

---

## 5. Implications for “full heap drop”

Heap `weight_` is **not** required by topology. It is required by:

1. **nocmodl ABI** — `pnt_receive(Point_process*, double*, flag)` and `net_send` capturing `_w` as SelfEvent identity.  
2. **FOR_NETCONS** — generated code walks other NetCons’ `weight_` bases (or needs a CoreNEURON-like weight perm).  
3. **INITIAL** — `pnt_receive_init(..., double*, ...)`.

CoreNEURON already dropped (1) by generating `weight_index` receive. NEURON default builds use nocmodl, so (1) is a **codegen/ABI** project unless we keep a permanent materialize shim (SoA → scratch buffer that is *not* per-NetCon heap — e.g. thread-local scratch of max arity, *if* SelfEvent identity is always `weight_index` / NetCon id, never the scratch pointer).

That last sentence is the real fork for “full drop without full GPU”:

- **Path H (heap-free, nocmodl kept):** SelfEvent and FOR_NETCONS never store/compare `double* weight_`; only indices; scratch is ephemeral and never identity.  
- **Path C (CoreNEURON ABI):** change generated receive to `weight_index` (nocmodl or NMODL project).  

Either can be a PR series; neither is required to *open* a design/review PR of dual-write+sort.

---

## 6. Discussion anchors (for back-and-forth)

1. **Should fanout order array stay `NetCon*` or become integer NetCon SoA rows?**  
   Integers match GPU/CoreNEURON export; pointers ease dual-write with DiscreteEvent queue.
2. **Should NetCon SoA physical order follow target-thread, source, or construction?**  
   Recommendation: **target-thread primary** (weights co-located); fanout stays a separate index range (CoreNEURON pattern).
3. **Is rank-local target enough, or must we also assert same-thread at connect?**  
   NEURON already delivers to `PP2NT(target)`; cross-thread NetCon is allowed as interthread send (same as CoreNEURON).
