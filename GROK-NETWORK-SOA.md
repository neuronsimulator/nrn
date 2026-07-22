# Grok handoff: NEURON CPU network SoA (`cpu_net_soa`)

Use this file when starting a **new** Grok session rooted in `~/neuron/cpu_net_soa`.

---

## Status (2026-07)

| Phase | Status | Notes |
|-------|--------|--------|
| **0** Spec | **Done** | `doc/network-soa-phase0.md` + layered `doc/network-soa/` |
| **1** PointProcess + Weight SoA dual-write | **Done** | Handles + create/destroy |
| **2** NetCon SoA; HOC weight → Weight SoA | **Done** | SoA HOC-primary; heap MOD scratch |
| **3** PreSyn SoA + fanout `NcIndex`/`NcCount` | **Done** | Rebuild at sort / `init_events` + lazy on spike |
| **4** SelfEvent `weight_index` + receive-by-index | **Done** | Still `pnt_receive(..., double*, flag)` (nocmodl) |
| **Sort wiring** | **Done** | Network in `nrn_ensure_model_data_are_sorted` |
| **Heap policy** | **Settled (no full free yet)** | Long-lived `weight_` required for `net_send(_w)` / FOR_NETCONS / INITIAL |
| **SaveState / BBSaveState dual-write** | **Done** | SoA values + NetCon-index SelfEvent identity |
| **5** GPU net buffers | **Out of scope** | After SoA is reviewable / on master |

**Developer docs (preferred reading order):** `doc/network-soa/README.md` → topology → dual-write → sort → phase0.

### Known gaps

- Full free of `NetCon::weight_`: needs FOR_NETCONS + INITIAL + SelfEvent never keying on temp `double*` (see `doc/network-soa/dual-write-and-heap.md`).
- Default codegen **nocmodl**; CoreNEURON-style `weight_index` receive is a separate ABI project.
- Fanout: SoA ranges preferred when sorted; `dil_` still filled and used as rebuild source / fallback (**fanout authority** not fully flipped — see topology doc).
- `InputPreSyn` thin gid→fanout deferred (phase0 §5.5.1).

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
| NetCon deliver | SoA → long-lived `weight_` → `pnt_receive` → SoA | Heap is MOD scratch; **required** so `net_send(..., _w)` keeps SelfEvent identity |
| NetCon deliver (FOR_NETCONS) | Same + sync all NetCons on target | MOD walks other NetCon `weight_` pointers |
| `pnt_receive_init` / HOC INITIAL | heap buffer + SoA sync | Keep until INITIAL uses index |
| SaveState | NetCon obj index + SoA weights | Dual-write restore sync done; `weight2netcon` remains live-queue helper |
| BBSaveState | DEList ncindex + SoA weights | SelfEvent match heap or weight_index; bind both on restore |
| HOC `weight[i]` | SoA `data_handle` | Already SoA-primary |

**Do not free `weight_` until** FOR_NETCONS, SaveState, and INITIAL no longer need stable heap bases. Prefer short-lived materialize everywhere else (current default for simple deliver).

Native nocmodl `weight_index` ABI is **out of scope** for this branch; keep materialize around generated `pnt_receive(double*)`.

## Recommended next work

1. ~~Sort + SaveState + BBSaveState dual-write~~ (done).
2. **Decide PR posture:** draft/review PR of dual-write+sort (no claim “land for perf yet”) vs private until heap-free / GPU (see `doc/network-soa/dual-write-and-heap.md` §5).
3. If pursuing heap-free: FOR_NETCONS index view + INITIAL by index + SelfEvent identity without heap pointer (doc L2 roadmap).
4. If pursuing authority cleanup: make fanout SoA sole hot-path truth; stop using `dil_` on spike (topology L1).
5. GPU Phase 5 only after SoA shape is stable enough to upload the same columns.

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

Phases 0–4 dual-write, sort wiring, SaveState/BBSaveState dual-write done.
Read doc/network-soa/README.md (topology, heap policy, sort). weight_ remains
MOD scratch under nocmodl. Next: PR posture, or FOR_NETCONS/fanout-authority
work toward heap-free — not GPU on this branch unless rebasing after SoA.
```

---

## Old GPU-native context

Stage 1 `NetReceiveBuffer` on gpu-native remains a valid **pattern**; indexing aligns with SoA after merge. See `~/neuron/nrngpu/GROK-GPU-NATIVE.md`.
