# Agent rules — NEURON CPU network SoA adoption

Full handoff and starting prompt: **`GROK-NETWORK-SOA.md`** (read on new sessions).

## Workspace

- Repo worktree: `~/neuron/cpu_net_soa`.
  - `local/cpu-network-soa` — dual-write PR #3822 (keep green with master).
  - `local/cpu-net-soa-heap-free` — heap-free follow-on (**branch only**, no PR); rebase onto PR tip.
- Primary git object store: `~/neuron/nrngpu` — commit from **this** worktree cwd.
- Sibling GPU track (paused network buffers): `~/neuron/nrngpu` @ `local/gpu-native-qualification`.

## Build

```bash
source ~/neuron/bin/nrnenv nrngpu build-cpu-net-soa   # create on first session if missing
# or: mkdir -p ~/neuron/cpu_net_soa/build && cd build && cmake .. -DNRN_ENABLE_GPU=OFF ...
```

Prefer CPU-only or default GPU-off builds until integration explicitly needs GPU mirrors.

## Scope

- Network SoA: `Point_process`, `NetCon`, `PreSyn`, `weights`, `SelfEvent` in `neuron::container` style.
- HOC wrappers as permutation-stable handles over backing store — **not** a second pointer graph.
- **Heap-free branch:** CoreNEURON-shaped sim path **plus** host packing / dynamic `nthread` between runs; full NEURON edit epoch. Charter: `doc/network-soa/heap-free.md`.
- **Out of scope:** Stage 2/3 GPU `net_buf_receive`, ringtest GPU network buffers (resume after SoA merges to master).

## Execute, don’t delegate

Run builds and tests yourself (`ctest`, ringtest CPU spike parity). Do not tell the user what to run unless blocked.

## Key references

| Topic | Path |
|-------|------|
| Handoff | `GROK-NETWORK-SOA.md` |
| Heap-free charter | `doc/network-soa/heap-free.md` |
| Phase 0 scaffold | `doc/network-soa-phase0.md` |
| Node/mechanism SoA pattern | `src/neuron/container/soa_container.hpp`, `data_handle.hpp` |
| PreSyn `thvar_` handle (prototype) | `src/nrncvode/netcon.h` |
| CoreNEURON layout reference | `src/coreneuron/sim/multicore.hpp`, `network/netcon.hpp` |
| nrncore export (prior art) | `src/nrniv/nrncore_write/` |