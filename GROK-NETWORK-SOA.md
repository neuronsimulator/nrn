# Grok handoff: NEURON CPU network SoA (`cpu_net_soa`)

Use this file when starting a **new** Grok session rooted in `~/neuron/cpu_net_soa`.

---

## Why a separate worktree

Network SoA is a focused CPU/infrastructure PR. It should **not** carry the GPU-native qualification commit stack (`local/gpu-native-qualification`).

| Worktree | Branch | Purpose |
|----------|--------|---------|
| `~/neuron/cpu_net_soa` | `local/cpu-network-soa` | Network SoA → PR to **master** |
| `~/neuron/nrngpu` | `local/gpu-native-qualification` | Mechanism GPU, Stage 1 buffer plumbing; Stages 2–3 **paused** until SoA lands |

Created with:

```bash
cd ~/neuron/nrngpu
git worktree add ~/neuron/cpu_net_soa -b local/cpu-network-soa origin/master
```

---

## Decision record (2026-07-08)

- **Pause** ringtest GPU network buffers (Stage 2 codegen, Stage 3 wire-up).
- **Discard** unstaged Stage 2 WIP on `gpu-native-qualification` (reverted).
- **Adopt** CoreNEURON-shaped network SoA on NEURON CPU first, with HOC types as wrappers over `data_handle` / `soa` backing store (same model as nodes/mechanisms).
- **Resume** GPU network buffers after SoA merges to master; gpu-native branch rebases then.

---

## North star

One CPU backing store for integration-hot network data:

- `std::vector` columns + permutation indices
- `data_handle` / `owning_handle` stable across permute
- `NrnThread` = lightweight slice (`offset` + count) into contiguous regions — **no duplicate CPU copies**
- HOC extras (`Object*`, recording, …) in sidecars keyed by handle

CoreNEURON layout is the **integration reference**; NEURON adds live create/delete/permute + interpreter compatibility.

---

## Architecture sketch

```text
HOC / Python (NetCon, PointProcess, PreSyn API)
        │
        ▼
Wrappers (handle / owning_handle into network SoA)
        │
        ▼
neuron::container::soa<...>  (weights, pntproc, netcon, presyn, …)
        │
        ▼
psolve hot path (deliver, threshold, fanout) — index-based
        │
        ▼
(future) GPU upload of same columns after merge with gpu-native track
```

---

## Phase plan (summary)

| Phase | Content | Gate |
|-------|---------|------|
| **0** | Field tags, handle typedefs, thread slicing — `doc/network-soa-phase0.md` | Spec done (authoritative) |
| **1** | `Point_process` + `weights` SoA scaffold under `src/neuron/container/network/` | handles survive permute (unit test) |
| **2** | `NetCon` SoA, HOC `weight()` → flat weights | CPU ringtest delivery |
| **3** | `PreSyn` `nc_index`/`nc_cnt` fanout | CPU spike parity @ 100 ms |
| **4** | `SelfEvent` indices, `pnt_receive(weight_index)` | ExpSyn @ 1.025 ms CPU |
| **5** | (gpu-native track) net buffers Stages 2–3 | GPU parity |

---

## Parallel work (not blocking this branch)

On `local/gpu-native-qualification`:

- Mechanism GPU gates A–E (ringtest mods).
- `-no-netcon` ringtest + APCount for NEURON vs CoreNEURON perf without `NET_RECEIVE`.

---

## Build (first session)

```bash
cd ~/neuron/cpu_net_soa
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$PWD/install -DNRN_ENABLE_MPI=ON
ninja && ninja install
source ~/neuron/bin/nrnenv nrngpu build-cpu-net-soa   # add nrnenv alias if helpful
```

Adjust `nrnenv` path to `~/neuron/cpu_net_soa/build` — agent may create `build-cpu-net-soa` on first configure.

---

## Tests (CPU gates)

```bash
# After Phase 3+
cd build && ctest -R ringtest --output-on-failure

# Spike parity (single host)
# mpiexec -n 1 ./path/to/special -nobanner ringtest.hoc
```

---

## Starting prompt (paste into new `cpu_net_soa` session)

```
Read ~/neuron/cpu_net_soa/GROK-NETWORK-SOA.md, AGENTS.md, and doc/network-soa-phase0.md.

Repo: ~/neuron/cpu_net_soa, branch local/cpu-network-soa (from origin/master).
Sibling: ~/neuron/nrngpu @ local/gpu-native-qualification (GPU track; network buffers paused).

Task: Write the Phase 0 design spec for network SoA — field tags, handle types,
per-thread slicing, HOC sidecar policy, invalidation rules. Then scaffold Phase 1
(Point_process + weights container) under src/neuron/container/network/.

Follow the node/mechanism DataHandle pattern (soa_container.hpp, data_handle.hpp).
PreSyn thvar_ data_handle is the existing network prototype. CoreNEURON multicore.hpp
is the integration layout reference.

Do not implement GPU net_buf_receive on this branch.
```

---

## Old GPU-native context

Stage 1 `NetReceiveBuffer` on gpu-native (`1f05cbc19`) remains valid **pattern**; indexing will align with SoA after merge. See `~/neuron/nrngpu/GROK-GPU-NATIVE.md`.