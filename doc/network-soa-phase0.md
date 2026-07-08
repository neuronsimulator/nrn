# Network SoA — Phase 0 design spec (draft scaffold)

**Status:** Draft for first implementation session. Expand into reviewable spec before Phase 1 coding.

**Branch:** `local/cpu-network-soa`  
**Base:** `origin/master`  
**Blocked downstream:** GPU network buffers on `local/gpu-native-qualification` (Stages 2–3)

---

## 1. Problem

NEURON network objects (`Point_process`, `NetCon`, `PreSyn`, `SelfEvent`) use pointer-heavy layouts suited to HOC interactivity. CoreNEURON GPU integration assumes flat per-thread arrays (`pntprocs`, `weights`, `netcons`, `presyns`) with integer indices.

Bridging with per-event shims on the legacy layout does not scale. **One SoA backing store** with HOC wrappers as handles matches the existing node/mechanism container model.

---

## 2. Goals

- Single CPU copy in `std::vector` columns; permutation-stable `data_handle` / `owning_handle`.
- `NrnThread` holds offsets into contiguous thread slices (no per-thread heap graphs).
- HOC API unchanged at the interpreter boundary (`weight()`, `loc()`, … read/write SoA).
- CPU `psolve` hot path uses indices (CoreNEURON-shaped), not `double* weight_` / `dil_` walks.
- GPU track can upload the same columns after SoA merges (out of scope for Phase 0–4).

## 3. Non-goals (Phase 0–4)

- GPU `net_buf_receive` codegen or Stage 3 runtime wire-up.
- Full `InputPreSyn` / MPI multisend redesign (defer to post–Phase 3 milestone).
- Removing HOC `Object*` identity — sidecars remain.

---

## 4. Reference layouts

### 4.1 CoreNEURON (integration hot)

| Entity | Key fields |
|--------|------------|
| `Point_process` | `_i_instance`, `_type`, `_tid` |
| `weights` | flat `double[n_weight]` |
| `NetCon` | `target_`, `u.weight_index_`, `delay_`, `active_` |
| `PreSyn` | `nc_index_`, `nc_cnt_`, `thvar_index_`, `threshold_`, `gid_` |
| `SelfEvent` | `target_`, `weight_index_`, `flag_` |

See `src/coreneuron/sim/multicore.hpp`, `src/coreneuron/network/netcon.hpp`.

### 4.2 NEURON today (to replace on hot path)

| Entity | Hot-path problem |
|--------|------------------|
| `Point_process` | `sec`, `node`, `prop`, `ob`, … pointers |
| `NetCon` | `double* weight_` heap per netcon |
| `PreSyn` | `NetConPList dil_`, fat HOC fields |
| `SelfEvent` | `double* weight_` |

See `src/nrnoc/section_fwd.hpp`, `src/nrncvode/netcon.h`.

### 4.3 NEURON precedent (already SoA-friendly)

- Nodes / mechanisms: `src/neuron/container/soa_container.hpp`, `data_handle.hpp`
- `PreSyn::thvar_` as `data_handle<double>`; GPU threshold uses `thvar_row` vs `nt->_node_data_offset` (`src/nrncvode/netcvode.cpp`)

---

## 5. Proposed containers (to decide in Phase 0)

### 5.1 Storage location

**Proposal:** extend `neuron::model()` with network storages (or `neuron::container::network::` namespace), analogous to `node_data()` and mechanism storage.

```cpp
// Illustrative — names TBD in spec
struct Model {
    container::Node::storage& node_data();
    container::network::PointProcessStorage& point_processes();
    container::network::WeightStorage& weights();
    container::network::NetConStorage& netcons();
    container::network::PreSynStorage& presyns();
};
```

### 5.2 Field tags (integration columns)

**Point_process row**

| Tag | Type | Notes |
|-----|------|-------|
| `Instance` | `int` | mechanism SoA row (`_i_instance`) |
| `MechType` | `int` | `_type` |
| `ThreadId` | `int` | `_tid` |

**Weight storage**

| Tag | Type | Notes |
|-----|------|-------|
| `Value` | `double` | flat array; NetCon owns `weight_index` base |

**NetCon row**

| Tag | Type | Notes |
|-----|------|-------|
| `TargetPnt` | `int` | index into point_process SoA |
| `WeightIndex` | `int` | into weights |
| `WeightCount` | `int` | `pnt_receive_size` |
| `Delay` | `double` | |
| `Active` | `bool`/`int` | |
| `PreSynIndex` | `int` | source presyn row, or -1 |

**PreSyn row**

| Tag | Type | Notes |
|-----|------|-------|
| `ThVar` | `data_handle<double>` or `int` row | prefer handle if already modern |
| `Threshold` | `double` | |
| `Gid` | `int` | |
| `NcIndex` | `int` | start index in netcon array |
| `NcCount` | `int` | fanout count |

**SelfEvent row** (if queued structurally; else event pool)

| Tag | Type | Notes |
|-----|------|-------|
| `TargetPnt` | `int` | |
| `WeightIndex` | `int` | |
| `Flag` | `double` | |

### 5.3 HOC sidecars (not in integration columns)

Keyed by `owning_handle` or stable id:

- `Object* ob`
- `IvocVect* tvec_`, recording state
- `HocCommand* stmt_`
- Cached `Section*` (invalidate on `tree_changed`)

---

## 6. Thread slicing

Mirror node model:

```text
Global SoA:  [ row 0 | row 1 | ... | row N-1 ]
             |← thread 0 →|← thread 1 →|
```

Per `NrnThread` (new fields, names TBD):

- `_pntproc_offset`, `_pntproc_count`
- `_netcon_offset`, `_netcon_count`
- `_presyn_offset`, `_presyn_count`
- `_weight_offset`, `_weight_count` (if weights partitioned per thread)

Permutation chosen so each thread region stays contiguous after `nrn_threads_create` / cell partitioning.

---

## 7. Handle API (wrappers)

| HOC-facing type | Wrapper holds | Example accessor |
|-----------------|---------------|------------------|
| `Point_process` | `PointProcess::owning_handle` | `prop()` → resolve via `_type` + `_i_instance` |
| `NetCon` | `NetCon::owning_handle` | `weight(i)` → `weights[weight_index+i]` |
| `PreSyn` | `PreSyn::owning_handle` | `threshold` → SoA column |

**CPU delivery signature (target):**

```cpp
pnt_receive[type](pnt_handle, weight_index, flag);
```

Align with CoreNEURON; migrate from `(Point_process*, double* _args, double flag)`.

---

## 8. Invalidation and sorting

- Topology / `define_shape` → `mark_as_unsorted()` on network containers (like `node_data()`).
- `tree_changed` → refresh sidecar caches; optional full network reorder pass.
- `psolve` / integration entry: `nrn_ensure_model_data_are_sorted()` includes network containers when fanout order matters.
- **Freeze policy:** same frozen-token pattern as mechanisms during sorted integration phases (TBD).

---

## 9. Migration strategy

| Step | Action |
|------|--------|
| M1 | Dual-write: create SoA row when creating legacy object |
| M2 | Dual-read: hot path reads SoA; HOC still works via wrapper |
| M3 | Remove hot-path pointer fields (`weight_`, `dil_` iteration) |
| M4 | Sidecars only for interpreter |

Order: **weights + Point_process → NetCon → PreSyn → SelfEvent**.

---

## 10. Phase 0 deliverables (checklist)

- [ ] Final field tag list per container (section 5.2)
- [ ] `Model` ownership and file layout (`src/neuron/container/network/`)
- [ ] Handle typedef names (`PointProcess::handle`, …)
- [ ] Thread offset fields on `NrnThread` / multicore
- [ ] Sidecar map design + invalidation rules
- [ ] `pnt_receive` signature migration plan
- [ ] SaveState / BBSaveState impact (minimal note for Phase 2+)
- [ ] Test plan: unit (permute), integration (ringtest CPU spikes)

---

## 11. Phase 1 first code (after spec sign-off)

```
src/neuron/container/network/
  point_process.hpp       # field tags, storage typedef, handle API
  point_process_data.cpp  # storage impl hooks
  weights.hpp
```

Hook point: point process allocation in existing `nrn_point_process` / mechanism insert path — allocate SoA row alongside today's `Prop`.

---

## 12. Open questions

1. Global vs per-thread weight pool — CoreNEURON uses per-thread `weights`; confirm for NEURON MPI.
2. When to permute network SoA relative to mechanism permute (`nrn_sort_mech_data`).
3. `NetCon` as `DiscreteEvent` subclass — keep for queue compatibility or separate event index?
4. `InputPreSyn` split — Phase 3 or later MPI milestone?

---

*Expand this document into the authoritative Phase 0 spec in the first `cpu_net_soa` session.*