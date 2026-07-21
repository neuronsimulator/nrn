# Grok handoff: NEURON CPU network SoA (`cpu_net_soa`)

Use this file when starting a **new** Grok session rooted in `~/neuron/cpu_net_soa`.

---

## Status (2026-07)

| Phase | Status | Notes |
|-------|--------|--------|
| **0** Spec | **Done** | `doc/network-soa-phase0.md` authoritative |
| **1** PointProcess + Weight SoA dual-write | **Done** | Handles + create/destroy |
| **2** NetCon SoA; HOC weight → Weight SoA | **Done** | Delivery materializes `double*` for MOD |
| **3** PreSyn SoA + fanout `NcIndex`/`NcCount` | **Done** | Rebuild at `init_events` + lazy on spike |
| **4** SelfEvent `weight_index` + receive-by-index | **Done** | M2: still `pnt_receive(..., double*, flag)` |
| **Sort wiring** | **Done** | Network in `nrn_ensure_model_data_are_sorted` (thread slices, weight repack) |
| **Heap-drop policy** | **In progress** | Short-lived materialize on deliver; heap kept for FOR_NETCONS / INITIAL |
| **SaveState dual-write** | **Done** | Weight save/restore via SoA; SelfEvent identity NetCon index + weight_index |
| **5** GPU net buffers | **Out of scope** | `local/gpu-native-qualification` after merge |

**Dual-write complete for the CoreNEURON-shaped data plane.** Network SoA participates in the global sort gate. Legacy pointers/`dil_`/`weight_` heap remain for interpreter, queue, and nocmodl ABI where required.

### Known gaps (do not “just drop heap” yet)

- Long-lived `NetCon::weight_` heap still allocated; hot-path deliver uses **short-lived materialize** unless the target type has `FOR_NETCONS` (needs stable heap bases). Full drop needs SaveState index keys + INITIAL/FOR_NETCONS migration.
- Default codegen is **nocmodl** (`NRN_ENABLE_NMODL=OFF`); native `weight_index` MOD ABI is a separate project.
- `InputPreSyn` / thin remote gid→fanout deferred (see phase0 §5.5.1).
- BBSaveState still uses weight pointer equality for SelfEvent binding (Commit B).
- Full `ctest -j N` needs a complete build/install; judge network work with filters below first.

### Default codegen

**nocmodl** (not NMODL). SoA weights work via materialize-around-`pnt_receive`. Native `weight_index` in MOD requires a separate nocmodl/ABI project.

---

## Why a separate worktree

Network SoA is a focused CPU/infrastructure PR. It should **not** carry the GPU-native qualification commit stack (`local/gpu-native-qualification`).

| Worktree | Branch | Purpose |
|----------|--------|---------|
| `~/neuron/cpu_net_soa` | `local/cpu-network-soa` | Network SoA → PR to **master** |
| `~/neuron/nrngpu` | `local/gpu-native-qualification` | Mechanism GPU, Stage 1 buffer plumbing; Stages 2–3 **paused** until SoA lands |

---

## North star

One CPU backing store for integration-hot network data:

- `std::vector` columns + permutation indices
- `data_handle` / `owning_handle` stable across permute
- `NrnThread` = lightweight slice (`offset` + count) — **no duplicate CPU copies**
- HOC extras in sidecars keyed by handle

CoreNEURON layout is the **integration reference**.

---

## Architecture (current dual-write)

```text
HOC / Python
    │
    ▼
Legacy shells (Point_process*, NetCon, PreSyn) + owning_handle _soa
    │
    ▼
neuron::container::network::{PointProcess,Weight,NetCon,PreSyn}
    │
    ▼
Hot path: fanout order + weight_index → materialize → pnt_receive(double*)
```

Key paths:

| Path | File / API |
|------|------------|
| Containers | `src/neuron/container/network/*.hpp` |
| Model ownership | `src/neuron/model_data.hpp` |
| PP dual-write | `Point_process::_soa`, `nrn_point_process_soa_sync` |
| NetCon / weights | `NetCon::_soa`, `weight_soa_`, `soa_sync` |
| Fanout | `PreSyn::ensure_fanout_order`, global NetCon* order |
| Sort / repack | `network_soa_sort.cpp`, `sort_network_data` in ensure_sorted |
| Receive by index | `nrn_pnt_receive_by_weight_index` |

---

## Sort wiring (implemented)

`nrn_ensure_model_data_are_sorted()` now freezes and sorts network containers after nodes + mechanisms:

1. **PointProcess** — partition by `ThreadId`; set `cache.thread[i].point_process_offset`
2. **Weight** — repack contiguous per-NetCon blocks ordered by target thread; set `weight_offset`
3. **NetCon** — same order as weight packing; set `netcon_offset`; refresh dual-write indices
4. **PreSyn** — partition by thread; rebuild `NcIndex`/`NcCount` fanout; set `presyn_offset`

Implementation: `src/nrncvode/network_soa_sort.cpp`, `neuron/container/network/sort.hpp`.

## Heap-drop policy (decision)

| Path | Weight source | Notes |
|------|---------------|--------|
| NetCon deliver (no FOR_NETCONS) | SoA → **temp buffer** → `pnt_receive` | Hot path; heap not required for content |
| NetCon deliver (FOR_NETCONS) | SoA → long-lived `weight_` heaps | MOD walks other NetCon `weight_` pointers |
| `pnt_receive_init` / HOC INITIAL | heap buffer + SoA sync | Keep until INITIAL uses index |
| SaveState | NetCon obj index + SoA weights | Dual-write restore sync done; `weight2netcon` remains live-queue helper |
| BBSaveState | heap pointer match for SelfEvent | Next: same NetCon-index policy as SaveState |
| HOC `weight[i]` | SoA `data_handle` | Already SoA-primary |

**Do not free `weight_` until** FOR_NETCONS, SaveState, and INITIAL no longer need stable heap bases. Prefer short-lived materialize everywhere else (current default for simple deliver).

Native nocmodl `weight_index` ABI is **out of scope** for this branch; keep materialize around generated `pnt_receive(double*)`.

## Recommended next work

1. ~~Wire network into `nrn_ensure_model_data_are_sorted`~~ (done).
2. ~~Heap-drop policy + short-lived materialize on simple deliver~~ (done; full free deferred).
3. ~~SaveState dual-write restore + SelfEvent NetCon-index identity~~ (done).
4. BBSaveState: SoA weight sync + SelfEvent NetCon-index binding (same policy as SaveState).
5. FOR_NETCONS / INITIAL without long-lived heap; then drop `weight_` allocation.
6. Merge toward master; GPU track rebases for Phase 5-style buffers.

---

## Build

```bash
cd ~/neuron/cpu_net_soa
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$PWD/install \
  -DNRN_ENABLE_MPI=ON -DNRN_ENABLE_TESTS=ON \
  -DNRN_ENABLE_CORENEURON=OFF -DNRN_ENABLE_NMODL=OFF
cmake --build . --parallel
# Prefer install for integration tests that need share/hoc
cmake --build . --target install
```

---

## Tests (CPU gates)

```bash
cd build

# Unit: SoA containers + dual-write helpers
./bin/test/testneuron '[network]' --reporter compact
./bin/test/testneuron '[data_structures]' --reporter compact

# Integration delivery gate (also under ctest pytest group)
# Requires PYTHONPATH / build env for neuron
python -m pytest ../test/pytest/test_network_soa_delivery.py -v

# Broader (env-sensitive)
ctest -j 4 -R 'unit_tests::testneuron|network_soa|pytest' --output-on-failure
# Full ctest: complete install first; CoreNEURON jobs skipped when disabled
```

---

## Starting prompt (new session)

```
Read ~/neuron/cpu_net_soa/GROK-NETWORK-SOA.md, AGENTS.md, and doc/network-soa-phase0.md.

Repo: ~/neuron/cpu_net_soa, branch local/cpu-network-soa.
Sibling: ~/neuron/nrngpu @ local/gpu-native-qualification (GPU network buffers paused).

Phases 0–4 dual-write + network sort wiring + SaveState dual-write are done.
Heap-drop policy: short-lived materialize on simple deliver; keep weight_
for FOR_NETCONS / INITIAL / BBSaveState. Next: BBSaveState NetCon-index
SelfEvent binding, or full heap free preconditions — not GPU on this branch.
```

---

## Old GPU-native context

Stage 1 `NetReceiveBuffer` on gpu-native remains a valid **pattern**; indexing aligns with SoA after merge. See `~/neuron/nrngpu/GROK-GPU-NATIVE.md`.
