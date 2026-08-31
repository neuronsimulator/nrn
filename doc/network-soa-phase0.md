# Network SoA — Phase 0 design spec

**Status:** Authoritative for `local/cpu-network-soa`. Phases 0–4 dual-write + network sort wiring implemented; see `GROK-NETWORK-SOA.md` for status and gaps.  
**Branch:** `local/cpu-network-soa` (from `origin/master`)  
**Sibling (paused):** `~/neuron/nrngpu` @ `local/gpu-native-qualification` — GPU network buffers Stages 2–3  
**Handoff:** `GROK-NETWORK-SOA.md`

---

## 1. Problem

NEURON network objects (`Point_process`, `NetCon`, `PreSyn`, `SelfEvent`) use pointer-heavy layouts suited to HOC interactivity:

| Entity | Hot-path problem |
|--------|------------------|
| `Point_process` | `sec`, `node`, `prop`, `ob`, `presyn_`, `_vnt` pointers |
| `NetCon` | per-NetCon `double* weight_` heap allocation |
| `PreSyn` | `NetConPList dil_` + fat HOC fields (`Object*`, recording, …) |
| `SelfEvent` | `double* weight_` into NetCon heap |

CoreNEURON GPU / multicore integration assumes **flat per-thread arrays** with integer indices (`pntprocs`, `weights`, `netcons`, `presyns`). See `src/coreneuron/sim/multicore.hpp`, `src/coreneuron/network/netcon.hpp`.

Bridging with per-event shims on the legacy layout does not scale. **One SoA backing store** with HOC wrappers as permutation-stable handles matches the existing node/mechanism container model (`src/neuron/container/soa_container.hpp`, `data_handle.hpp`).

**Existing network prototype:** `PreSyn::thvar_` is already `neuron::container::data_handle<double>` (`src/nrncvode/netcon.h`). Threshold evaluation and GPU threshold use stable handles / row offsets relative to `NrnThread::_node_data_offset`.

---

## 2. Goals

1. Single CPU copy of integration-hot network data in `std::vector` columns.
2. Permutation-stable `data_handle` / `owning_handle` (same lifetime model as `Node`).
3. `NrnThread` (and `neuron::cache::Thread`) hold **offsets + counts** into contiguous global slices — no per-thread duplicate heap graphs.
4. HOC / Python API unchanged at the interpreter boundary (`weight()`, `loc()`, `NetCon.active`, …).
5. CPU `psolve` hot path becomes index-based (CoreNEURON-shaped), not `double* weight_` / `dil_` walks.
6. After SoA merges to master, the GPU track can upload the same columns (out of scope here).

## 3. Non-goals (Phases 0–4 on this branch)

| Out of scope | Resume when |
|--------------|-------------|
| GPU `net_buf_receive` codegen / Stage 3 wire-up | SoA on master; rebase `gpu-native-qualification` |
| Remote input / `InputPreSyn` split; MPI multisend redesign | After cpu-net-soa (see §5.5.1) |
| Removing HOC `Object*` identity | Never — sidecars remain |
| Changing public HOC/Python method signatures | Not required |

---

## 4. Reference layouts

### 4.1 CoreNEURON (integration hot)

From `coreneuron::NrnThread` and network headers:

| Entity | Key fields |
|--------|------------|
| `Point_process` | `_i_instance`, `_type`, `_tid` |
| `weights` | flat `double[n_weight]`; `NetCon.u.weight_index_` base |
| `NetCon` | `target_`, `u.weight_index_`, `delay_`, `active_` |
| `PreSyn` | `nc_index_`, `nc_cnt_`, `thvar_index_`, `threshold_`, `gid_` |
| `SelfEvent` | `target_`, `weight_index_`, `flag_` |

Weights for one NetCon are **contiguous** at `[weight_index, weight_index + pnt_receive_size)`.

### 4.2 NEURON node/mechanism precedent

```text
owning_handle  → owns a row; destructor frees row
handle         → non-owning; stable across permute; dies when owner dies
data_handle<T> → erases field identity; used for pointers into columns
```

Storage lives under `neuron::model()` (`Model::node_data()`, `Model::mechanism_data(type)`).  
Thread slice: `NrnThread::_node_data_offset` + `neuron::cache::Thread::{node,mechanism}_offset`.  
Sort gate: `nrn_ensure_model_data_are_sorted()` freezes tokens, permutes, rebuilds `neuron::cache::model`.

### 4.3 PreSyn `thvar_` prototype

- Construction takes `data_handle<double>` for the watched voltage (or related variable).
- `notify_when_handle_dies` disconnects when the underlying node dies.
- Do **not** re-introduce raw `double*` for threshold sources in new code.

---

## 5. Containers and field tags

### 5.1 Ownership and file layout

**Decision:** Network storages are members of `neuron::Model`, analogous to `m_node_data`.

```
src/neuron/container/network/
  point_process.hpp     # Phase 1 — tags, storage, handle API
  weights.hpp           # Phase 1
  netcon.hpp            # Phase 2
  presyn.hpp            # Phase 3
  self_event.hpp        # Phase 4 (if queued structurally)
```

```cpp
// neuron::Model (illustrative)
container::network::PointProcess::storage& point_processes();
container::network::Weight::storage&       weights();
container::network::NetCon::storage&       netcons();      // Phase 2
container::network::PreSyn::storage&       presyns();      // Phase 3
```

Namespace root: `neuron::container::network`.  
Each entity is a nested namespace with `field::*` tags, `storage`, `handle`, `owning_handle` — same shape as `neuron::container::Node`.

### 5.2 Point_process (Phase 1)

**Integration columns** (CoreNEURON-compatible):

| Tag | C++ type | Default | Meaning |
|-----|----------|---------|---------|
| `field::Instance` | `int` | `-1` | Mechanism SoA row (`_i_instance`) |
| `field::MechType` | `int` | `-1` | Mechanism type (`_type`) |
| `field::ThreadId` | `int` | `-1` | Owning `NrnThread` id (`_tid`) |

**Handle typedefs:**

| Name | Type |
|------|------|
| `PointProcess::handle` | `handle_interface<non_owning_identifier<storage>>` |
| `PointProcess::owning_handle` | `handle_interface<owning_identifier<storage>>` |

**Accessors on `handle_interface`:** `instance()`, `mech_type()`, `thread_id()`, plus `*_handle()` returning `data_handle` where useful for debugging / HOC bridges.

**Not in SoA (sidecars / dual-write legacy fields):** see §7.

### 5.3 Weights (Phase 1)

**Integration columns:**

| Tag | C++ type | Default | Meaning |
|-----|----------|---------|---------|
| `field::Value` | `double` | `0.0` | One weight scalar |

One **row** = one scalar weight entry. A NetCon with `cnt = pnt_receive_size[type]` owns a **contiguous block** of `cnt` rows.

| Name | Type |
|------|------|
| `Weight::handle` / `Weight::owning_handle` | same pattern as Node |

**Contiguity invariant (CoreNEURON-compatible):**

```text
weights[weight_index + k]  for k in [0, weight_count)
```

- Allocation API (Phase 1→2) must allocate **blocks** of size `weight_count` as consecutive rows.
- Permutation of the weight container must either:
  1. **Block-permute** (move whole NetCon groups and rewrite `NetCon::WeightIndex`), or
  2. **Repack** at sort time into contiguous groups (preferred at `nrn_ensure_model_data_are_sorted`).
- Freeing a NetCon frees its entire block (swap-with-last **block** or hole + repack).

**Why not one SoA row per NetCon with runtime array dim?** Weight counts differ by target mechanism type (`pnt_receive_size`). A flat pool matches CoreNEURON export (`nrncore_write`) and GPU upload.

**Hot-path access:** integer `weight_index` base (current row of first weight in the group), **not** a stable identifier, so delivery stays CoreNEURON-shaped. After any weight permute/repack, all `NetCon` weight indices are remapped. HOC `weight(i)` may use `data_handle<double>` obtained from the base row + array shift once the group is a single logical array, or `weights.get<Value>(weight_index + i)`.

### 5.4 NetCon (Phase 2 — tags fixed now)

| Tag | C++ type | Default | Meaning |
|-----|----------|---------|---------|
| `field::Target` | `int` | `-1` | Row in `PointProcess` storage (or sentinel) |
| `field::WeightIndex` | `int` | `-1` | Base row in `Weight` storage |
| `field::WeightCount` | `int` | `0` | `pnt_receive_size` for target type |
| `field::Delay` | `double` | `1.0` | Delivery delay (ms) |
| `field::Active` | `int` | `1` | `bool` as `int` (GPU-friendly, CoreNEURON style) |
| `field::SrcPreSyn` | `int` | `-1` | Source PreSyn row, or `-1` if none (management / reverse edge; see §5.4.1) |

**Optional later:** `field::ObjectId` only if HOC identity must be recovered from a row without a sidecar map.

**Event-queue identity:** `NetCon` remains a `DiscreteEvent` subclass for queue compatibility through Phase 2–3. The SoA row is the data plane; the C++ object is the control/HOC plane during dual-write. Long term the queued payload may shrink to `(type, row)` — not required for Phase 2 gate.

#### 5.4.1 NetCon linkage: `target_` and `src_` (pointers vs handles vs indices)

Today NEURON keeps a **mutual pointer graph**:

```text
PreSyn ──dil_──► NetCon* ──src_──► PreSyn
                    │
                    └──target_──► Point_process*
```

CoreNEURON’s integration layout does **not** retain that graph: `NetCon` has `target_` (struct pointer into `pntprocs`) and `weight_index_`; source is implicit because the NetCon sits in a PreSyn’s fanout range (`nc_index_` / `nc_cnt_`). There is no `src_` on the hot path.

**Decision for NEURON SoA — three layers, not one type for all uses:**

| Concern | Representation | Why |
|---------|----------------|-----|
| **Spike / delivery hot path** | Integer **SoA row indices** (`Target`, `WeightIndex`; fanout via PreSyn `NcIndex`/`NcCount`) | CoreNEURON-shaped, GPU-uploadable, no pointer chasing, contiguous scans |
| **Stable identity across permute / dual-write / HOC** | `PointProcess::handle` / `PreSyn::handle` (or `non_owning_identifier`) on the wrapper object | Same model as `Node::handle` — tracks row through permute without remapping by hand on every HOC touch |
| **Legacy shell (transitional)** | Keep `Point_process* target_` and `PreSyn* src_` on the C++ `NetCon` during dual-write | Existing disconnect / HOC / queue code keeps working until dual-read is complete |

**Do not use `data_handle` for `target_` / `src_`.**  
`data_handle<T>` is a stable reference to a **scalar field in a column** (e.g. `PreSyn::thvar_` → a `double` voltage). A NetCon’s target is a **row of another entity**, not a double. The correct SoA tools are:

- **row index `int`** after sort (integration),
- **`handle` / `owning_handle` / `non_owning_identifier`** when a stable cross-reference is needed outside the frozen sorted window.

**Directionality of the assembly (important):**

```text
Hot path (spike → targets):
  PreSyn  ──NcIndex/NcCount──►  NetCon rows  ──Target index──►  PointProcess row
                                    │
                                    └── WeightIndex ──► weight block

Reverse edge (management / HOC only):
  NetCon  ──SrcPreSyn index or PreSyn::handle──►  PreSyn
```

- **Forward fanout is authoritative** for delivery (replaces walking `dil_` as a pointer list).
- **`src_` / `SrcPreSyn` is a reverse edge**: disconnect, `replace_src`, mindelay queries, interpreter inspection. It is **not** required for CoreNEURON-shaped deliver. It may remain on the HOC wrapper / dual-write shell longer than `target_`, or stay as a SoA `int` column for convenience when destroying a PreSyn and sweeping its NetCons (until ranges alone suffice).

**Why not keep pointer mutuality as the long-term design?**

- Pointer graphs fight permutation, thread packing, and GPU mirrors.
- Two-way raw pointers duplicate topology already expressed by fanout ranges + `Target`.
- Mutuality is valuable for **interactive** NEURON; express it with **handles + indices**, not with a second heap of cross-pointers that own the topology.

**Phase timing:** Phase 2 dual-writes `Target` / weights while leaving `target_`/`src_` pointers valid; Phase 3 builds fanout ranges; hot path drops `dil_` walks and pointer fanout; pointer fields become optional wrappers over handles/indices and can shrink later without another layout break.

### 5.5 PreSyn (Phase 3 — tags fixed now)

| Tag | C++ type | Default | Meaning |
|-----|----------|---------|---------|
| `field::Threshold` | `double` | `10.0` | Spike threshold |
| `field::Gid` | `int` | `-1` | Output gid or −1 |
| `field::NcIndex` | `int` | `-1` | Start of fanout range in NetCon order array |
| `field::NcCount` | `int` | `0` | Fanout count |
| `field::OutputIndex` | `int` | `0` | CoreNEURON `output_index_` |
| `field::ThVarRow` | `int` | `-1` | Optional denormalized node-voltage row for threshold scan; **canonical source remains `data_handle` in sidecar / dual-write `thvar_`** |
| `field::ThreadId` | `int` | `-1` | Owning thread |

**Fanout model (replaces `dil_`):**

```text
Global NetCon order array (or NetCon SoA already sorted by source):
  netcons[NcIndex .. NcIndex+NcCount)

On spike: for i in [0, NcCount): deliver netcons[NcIndex+i]
```

Matches CoreNEURON `nc_index_` / `nc_cnt_` into `netcon_in_presyn_order_`. At sort time, NetCons are (re)ordered by source PreSyn so ranges are contiguous.

**Threshold source:** keep `data_handle<double>` (today’s `thvar_`) in the dual-write / sidecar plane. Optionally cache `ThVarRow` when the handle refers to node voltage for vectorized threshold checks.

#### 5.5.1 PreSyn vs remote input (`InputPreSyn`)

Two **roles** appear on a rank; CoreNEURON splits them into types, NEURON historically does not.

| Role | Job on this rank | Threshold / `thvar` / HOC recording? | Fanout (`NcIndex`/`NcCount`)? |
|------|------------------|--------------------------------------|-------------------------------|
| **Local / output PreSyn** | Detect spike, optional gid send, fan out local NetCons | Yes | Yes |
| **Remote / input source** | Map received `(gid, t)` to local NetCons | No | Yes |

CoreNEURON’s `InputPreSyn` is the second role reduced to `nc_index_` / `nc_cnt_` (plus optional multisend bookkeeping). NEURON today stores **full `PreSyn*`** in both `gid2out_` and `gid2in_` (`netpar.cpp`), so remote sources pay for `ConditionEvent`, threshold fields, and sidecars they never use. That cost matters when connectivity is dense and `gid2in` is large — the common case where **MPI_Allgather** (or compressed collectives) remains near-optimal because almost every rank needs almost every spike. Point-to-point / multisend sophistication (BlueGene-era) is orthogonal: it changes the **wire** path, not whether a remote source needs threshold state.

**Is the role still useful?** Yes. Remote delivery only needs:

```text
gid  →  (NcIndex, NcCount)  →  NetCon rows  →  targets / weights
```

**Must NEURON clone CoreNEURON’s dual class tree?** No. Prefer a **data-plane** split when the time comes:

1. **Local PreSyn SoA** — threshold columns + gid/output + fanout range + HOC sidecars.  
2. **Remote inputs** — thin **`gid → fanout range`** (dedicated small SoA or map), not a fat PreSyn and not a HOC-facing type.  
3. Spike exchange resolves gid → range → enqueue NetCons (or one lightweight input event holding the range + time).

That preserves CoreNEURON’s memory and export shape without a second interactive pointer graph. Optional fields on one “spike source” SoA are a weaker fit (mixed sort keys, threshold code special-cases).

**Timing (explicitly out of scope for this branch’s Phase 0–4):**

| When | What |
|------|------|
| Phases 1–2 | Ignore remote split |
| Phase 3 | Local fanout on existing PreSyn; `gid2in_` may stay fat PreSyn |
| **After** cpu-net-soa finishes (post–Phase 3/4 MPI milestone) | Thin remote representation; align `nrncore` / CoreNEURON `gid2in` |
| GPU net buffers | Upload the same fanout ranges; role split reduces payload |

**Design constraint for Phase 3:** build fanout so a later remote-input table can own the same style of `(NcIndex, NcCount)` ranges **without** restating NetCon layout. Do not implement `InputPreSyn` work on `local/cpu-network-soa`.

### 5.6 SelfEvent (Phase 4)

SelfEvents are short-lived queue items. Prefer **not** a long-lived SoA of all SelfEvents; instead:

| Field on event / pool object | Type | Meaning |
|------------------------------|------|---------|
| `target_pnt` | `int` | PointProcess row |
| `weight_index` | `int` | Into weights (same base as NetCon or NULL weights) |
| `flag` | `double` | NET_RECEIVE flag |

If a structural pool is useful later, tags match the above. Gate is `pnt_receive(weight_index)` migration (§9).

### 5.7 Tag summary by phase

| Phase | Container | Gate |
|-------|-----------|------|
| **0** | Spec (this doc) | Spec reviewed |
| **1** | `PointProcess` + `Weight` | Handles survive permute; dual-write on create/destroy |
| **2** | `NetCon` SoA; HOC `weight()` → flat Weight SoA | dual-write; handles survive permute |
| **3** | `PreSyn` SoA + fanout `NcIndex`/`NcCount` | dual-write; hot path uses fanout order |
| **4** | SelfEvent weight_index; `nrn_pnt_receive_by_weight_index` | dual-write M2 (double* still for MOD) |
| **5** | (gpu-native track) net buffers | GPU parity — **not this branch** |

---

## 6. Thread slicing

### 6.1 Layout

Mirror nodes:

```text
Global PointProcess SoA:  [  thread0 rows  |  thread1 rows  | ... ]
Global Weight SoA:        [  thread0 block |  thread1 block | ... ]
Global NetCon SoA:        [  thread0 rows  |  thread1 rows  | ... ]
Global PreSyn SoA:        [  thread0 rows  |  thread1 rows  | ... ]
```

### 6.2 Fields

**On `NrnThread` (or `neuron::cache::Thread` — prefer cache for offsets that only matter when sorted):**

| Field | Meaning |
|-------|---------|
| `_pntproc_offset` / `pntproc_count` | Slice of PointProcess storage |
| `_weight_offset` / `weight_count` | Slice of Weight storage |
| `_netcon_offset` / `netcon_count` | Slice of NetCon storage |
| `_presyn_offset` / `presyn_count` | Slice of PreSyn storage |

**Decision (open Q1 resolved):** Weights are **partitioned per thread** (CoreNEURON `nt.weights` / `nt.n_weight`). Cross-thread NetCon is already restricted; weight blocks live on the **target** thread of the Point_process.

**Hot-path address:**

```cpp
double* w = weight_storage.get_field_data<field::Value>() + nt.weight_offset;
// delivery uses absolute weight_index into global storage, or local index
//   local = absolute - nt.weight_offset
```

Absolute global indices simplify dual-write; local indices match CoreNEURON. Prefer **absolute** during dual-write, document conversion at nrncore export.

### 6.3 When slices are computed

During `nrn_ensure_model_data_are_sorted()` / network sort pass:

1. Assign each PointProcess to a thread (from cell partition / existing `_vnt`).
2. Permute PointProcess so thread regions are contiguous; set offsets.
3. Order NetCons by `(src_presyn, …)`; place on target thread of `Target`.
4. Pack weight blocks per NetCon into the target thread’s weight region; rewrite `WeightIndex`.
5. Order PreSyn by thread; build `NcIndex`/`NcCount` into the NetCon order.

---

## 7. HOC sidecar policy

### 7.1 Principle

Integration columns are the **only** data the hot path and (future) GPU need.  
Interpreter / recording / identity live in **sidecars** keyed by stable id, **not** a second pointer graph that owns network topology.

### 7.2 Keying

| Key | Use |
|-----|-----|
| `owning_handle` / `non_owning_identifier_without_container` | Primary: survives permute; dies with row |
| HOC `Object*` | External identity; points at wrapper that holds `owning_handle` |

Do **not** key sidecars by current SoA row integer alone without a stable id.

### 7.3 Sidecar contents by entity

**Point_process**

| Field | Notes |
|-------|-------|
| `Object* ob` | HOC object |
| `Section* sec`, `Node* node` | Location; invalidate on topology change |
| `Prop* prop` | Mechanism instance bridge during dual-write |
| `void* presyn_`, `nvi_`, `_vnt` | Until indices replace them |

During Phase 1 dual-write, the existing `struct Point_process` **is** the sidecar + legacy shell; SoA holds Instance/MechType/ThreadId. Migration moves fields out of the struct into maps/`owning_handle` wrappers over time.

**NetCon**

| Field | Notes |
|-------|-------|
| `Object* obj_` | HOC |
| `PreSyn* src_` | Dual-write reverse edge; long-term `SrcPreSyn` index and/or `PreSyn::handle` (§5.4.1) — not hot-path delivery |
| `Point_process* target_` | Dual-write; long-term `Target` index (+ optional `PointProcess::handle` on wrapper) |
| `double* weight_` | Dual-write alias into Weight SoA (then removed) |

**PreSyn**

| Field | Notes |
|-------|-------|
| `data_handle<double> thvar_` | **Keep** as canonical threshold source |
| `Object* osrc_`, `Section* ssrc_` | Source identity |
| `IvocVect* tvec_`, `idvec_`, `HocCommand* stmt_` | Recording |
| `NrnThread* nt_`, `hoc_Item* hi_th_` | Thread / threshold list |
| `NetConPList dil_` | Dual-write until `NcIndex`/`NcCount` |
| MPI / MUSIC / multisend unions | Sidecar or later phase |

### 7.4 What must never be duplicated as “second graph”

- Do not keep a parallel `std::vector<NetCon*>` fanout **and** `NcIndex`/`NcCount` as equally authoritative after Phase 3. One source of truth: SoA ranges.
- Do not allocate per-NetCon `new double[cnt]` after weight migration completes.

---

## 8. Invalidation and sorting

### 8.1 Unsorted triggers

| Event | Action |
|-------|--------|
| Create/destroy PointProcess, NetCon, PreSyn | `mark_as_unsorted()` on affected network container(s) |
| NetCon retarget / weight count change | weights + netcons unsorted |
| Topology / `define_shape` / `tree_changed` | Invalidate sidecar caches (`Section*`, node location); mark network unsorted if thread membership may change |
| `nrn_threads_create` / repartition | Full network reorder |
| Mechanism permute that changes Instance rows | Update PointProcess `Instance` column (or mark unsorted and fix at sort) |

Each network `storage` registers `set_unsorted_callback` → `neuron::cache::model.reset()` (same as nodes).

### 8.2 Sort / freeze policy

Extend `nrn_ensure_model_data_are_sorted()`:

1. Issue frozen tokens for node, mechanisms, **and** network containers.
2. If any network container is unsorted, run network permute/repack (§6.3).
3. Hold frozen tokens through integration like mechanisms.

**Freeze:** same `frozen_token_type` pattern — no row insert/delete while sorted token is held for that container.

### 8.3 Handle death

| Situation | Behavior |
|-----------|----------|
| `owning_handle` destroyed | Row freed; non-owning handles / `data_handle`s to that row become invalid (`operator bool` false) |
| PointProcess destroyed while NetCons target it | Existing NetCon disconnect rules; `Target = -1` |
| Weight block freed | NetCon weight fields cleared; HOC weight access errors |
| Node voltage dies under `thvar_` | Existing `notify_when_handle_dies` path |

### 8.4 Permutation vs mechanism sort (open Q2 resolved)

**Order inside global sort:**

1. Nodes (existing).
2. Mechanisms (existing) — Instance rows stable for this step’s PointProcess fixup.
3. **Network:** PointProcess (by thread), Weights (repack by NetCon), NetCon (by src PreSyn / thread), PreSyn (by thread) + fanout ranges.

PointProcess `Instance` is rewritten if mechanism permute moved rows (stable mech handles preferred when available).

---

## 9. `pnt_receive` signature migration

### 9.1 Today

```cpp
typedef void (*pnt_receive_t)(Point_process*, double* weight, double flag);
// POINT_RECEIVE(type, tar, w, f) (*pnt_receive[type])(tar, w, f)
```

### 9.2 Target (align CoreNEURON)

```cpp
// Conceptual end state
void pnt_receive(int type, int pnt_instance /* or pnt row */, int weight_index, double flag);
// body loads weights via global/thread weight base + weight_index
```

### 9.3 Migration steps

| Step | Action |
|------|--------|
| M1 | Dual-write weights into SoA; `NetCon::weight_` points into SoA (`data()` + index) so existing `pnt_receive` still gets `double*` |
| M2 | Delivery uses weight_index internally; still materializes `double*` for generated code |
| M3 | Codegen / MOD translation accepts weight_index (Phase 4); legacy pointer form kept until mods regenerated |
| M4 | Drop per-NetCon heap weights |

Phase 1 only needs M1 scaffolding (container + optional pointer into SoA). **No MOD codegen changes in Phase 1.**

---

## 10. SaveState / BBSaveState (note for Phase 2+)

| Concern | Impact |
|---------|--------|
| `NetConSave` weight pointer tables | Live helper `weight2netcon(double*)` remains; SaveState SelfEvent **file** identity is NetCon obj index + `weight_index2netcon`. Call `NetConSave::invalid()` when heap bases reallocate. |
| SaveState weight values | Save prefers Weight SoA; restore writes heap + SoA (`restorenet`). |
| `PreSynSave` / `hi_index_` | Keep index tables; rebuild on unsorted. |
| BBSaveState | Weight SoA sync on IO; SelfEvent DEList `ncindex` + heap/`weight_index` match (Commit B done). |
| Binary layout | SelfEvent line already stores `ncindex`; keep compatible. |

Minimal Phase 1 impact: none if PointProcess/Weight SoA is not yet referenced by SaveState.

---

## 11. Handle API and HOC wrappers

| HOC-facing type | Wrapper holds | Example |
|-----------------|---------------|---------|
| Point process object | `PointProcess::owning_handle` (+ legacy `Point_process*` during dual-write) | `instance()` → mech row |
| `NetCon` | `NetCon::owning_handle` | `weight(i)` → `weights[WeightIndex+i]` |
| `PreSyn` | `PreSyn::owning_handle` | `threshold` ↔ SoA column; `thvar_` stays `data_handle` |

**CPU delivery target:**

```cpp
// After Phase 4
pnt_receive[type](/* target identity */, weight_index, flag);
```

---

## 12. Migration strategy (execution order)

| Step | Action |
|------|--------|
| M1 dual-write | Allocate SoA row when creating legacy object |
| M2 dual-read | Hot path prefers SoA; HOC via wrapper/legacy |
| M3 remove hot-path pointers | Drop `weight_` heap, `dil_` iteration |
| M4 sidecars only | Interpreter extras only in sidecars |

**Order:** weights + Point_process → NetCon → PreSyn → SelfEvent.

---

## 13. Test plan

### 13.1 Unit (Phase 1)

| Test | Expectation |
|------|-------------|
| Create N PointProcess rows; set Instance/MechType/ThreadId | Round-trip via handles |
| `apply_reverse_permutation` on PointProcess storage | Handle logical values unchanged; storage order may change |
| Create weight block of size K; permute | Values via handles stable |
| Destroy `owning_handle` | Non-owning handle / data_handle invalid |
| `find_container_info` / `data_handle` promote | Weight `Value` and PointProcess fields discoverable |

### 13.2 Integration (Phase 2+)

| Gate | Command / check |
|------|-----------------|
| Phase 2 | CPU ringtest delivery (NetCon → ExpSyn) |
| Phase 3 | CPU spike parity @ 100 ms vs baseline |
| Phase 4 | ExpSyn receive @ 1.025 ms CPU |
| Regression | `ctest` network-related; SaveState smoke if touched |

### 13.3 Non-tests on this branch

- GPU `net_buf_receive`, ringtest GPU network buffers.

---

## 14. Phase 0 deliverables checklist

- [x] Final field tag list per container (§5.2–5.6)
- [x] `Model` ownership and file layout (`src/neuron/container/network/`)
- [x] Handle typedef names (`PointProcess::handle`, `Weight::owning_handle`, …)
- [x] Thread offset fields on `NrnThread` / cache (§6)
- [x] Sidecar map design + invalidation rules (§7–§8)
- [x] `pnt_receive` signature migration plan (§9)
- [x] SaveState / BBSaveState impact note (§10)
- [x] Test plan (§13)

---

## 15. Phase 1 code scaffold

```
src/neuron/container/network/
  point_process.hpp       # field tags, storage, handle API
  weights.hpp             # field tags, storage, handle API
```

Hook points (implementation after scaffold):

1. `Model` members + accessors + `find_container_info` + unsorted callbacks.
2. Unit test: permute survival.
3. Later: `nrn_point_process` / NetCon ctor dual-write (not required for empty scaffold compile).

---

## 16. Resolved design questions

| # | Question | Decision |
|---|----------|----------|
| 1 | Global vs per-thread weight pool | **Per-thread slices** of one global SoA (CoreNEURON-compatible); absolute indices during dual-write OK |
| 2 | Network permute vs mechanism permute | **After** nodes + mechanisms inside `nrn_ensure_model_data_are_sorted` |
| 3 | NetCon as `DiscreteEvent` | **Keep subclass** through Phase 2–3 for queue compatibility |
| 4 | `InputPreSyn` | **Role yes, dual class optional**; thin gid→fanout after cpu-net-soa — §5.5.1 |
| 5 | Weight contiguity under permute | **Repack at sort** into contiguous per-NetCon blocks; rewrite `WeightIndex` |
| 6 | PreSyn threshold | **Keep `data_handle`**; optional `ThVarRow` cache for scans |
| 7 | NetCon `target_` / `src_` | **Indices on hot path**; **handles** for stable/HOC refs; **not** `data_handle`; pointers dual-write only — §5.4.1 |

---

## 17. Out-of-scope reminder

Do **not** implement GPU `net_buf_receive` on `local/cpu-network-soa`. Stage 1 `NetReceiveBuffer` on the gpu-native branch remains a valid pattern; indexing will align after SoA merges.

---

*This document is the Phase 0 gate. Phase 1 implements PointProcess + Weight containers and proves handles survive permutation.*
