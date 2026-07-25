# Grok handoff: NEURON native GPU + heap-free network SoA

Use this file when starting a **new** Grok session rooted in `~/neuron/nrngpu`
on branch **`local/gpu-native-net-soa`**.

Related CPU docs (same tree after merge):

| Doc | Role |
|-----|------|
| `GROK-NETWORK-SOA.md` | Network SoA / dual-write history |
| `doc/network-soa/heap-free.md` | Weight-index ABI, packing A charter |
| `AGENTS.md` | Short agent rules for this branch |

---

## Why the UI still shows an old workspace name

Grok sessions are stored under `~/.grok/sessions/<encoded-cwd>/<session-id>/`.
The **working directory is fixed when the session is created**.

To work here:

1. Open **Open Folder → `~/neuron/nrngpu`**.
2. Start a **new** session (`/new`), not Resume on an old `core-neuron-gpu` /
   `cpu_net_soa` session unless that session’s cwd matches this tree.
3. Paste the **starting prompt** below (or: “Read `GROK-GPU-NATIVE.md` and continue”).

Session portability (Linux ↔ Mac): `~/neuron/notes/session-portability.md`.

---

## Canonical repo and branch

| Item | Value |
|------|--------|
| Repo | `~/neuron/nrngpu` (primary; commit here) |
| Branch | **`local/gpu-native-net-soa`** |
| Base | `local/cpu-net-soa-heap-free` (PR [#3826](https://github.com/neuronsimulator/nrn/pull/3826)) |
| GPU overlay | `local/gpu-native-qualification` (merged) |
| Integration merge | `e0dd78c18` (2026-07-24) |
| Build / install | `~/neuron/nrngpu/build-gpu` via `nrnenv` / `ninja` / `ninja install` |
| Do **not** wait for #3826 → master | Stack GPU work on this branch until a full native-GPU PR |

Upstream PR #3826 may stay open until native-GPU work gives it review weight. Treat
heap-free tip as the long-lived base; rebase onto master after #3826 lands if needed.

---

## Qualification goal

**HH GPU-native ringtest:** 688 spikes @ `tstop=100` with CPU parity.

Harness:

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_NATIVE_GPU_DEVICE_NONVINT=1
export NRN_GPU_BACKEND_TEST=native
export NRN_GPU_PERMUTE=2

cd ~/neuron/nrngpu/build-gpu/test/external_ringtest/neuron_gpu_native_mpi
# Rebuild special after install if needed:
#   rm -rf x86_64 && nrnivmodl .

./prcellstate_native_gpu.sh 32 0.025 0   # single step
./prcellstate_native_gpu.sh 32 1 0       # pre-spike baseline
./prcellstate_native_gpu.sh 32 1.025 0   # first NetCon event (Stage 2–3 gate)
./prcellstate_native_gpu.sh 32 1.025 1   # same + phase dumps
./prcellstate_native_gpu.sh 32 100       # long gate (688 spikes)
```

Source: `test/external/ringtest/prcellstate_native_gpu.sh` (copied into build tree at configure).

Compare:

```bash
python ~/models/82894/rdcellstate.py --ignore-unused \
  32_-t1.nrndat 32_-gpu-t1.nrndat
```

---

## Baseline after integration (2026-07-25)

On `local/gpu-native-net-soa` @ `e0dd78c18`, rebuild + install + `nrnivmodl` in harness dir:

| tstop | Result |
|-------|--------|
| **0.025** | CPU/GPU parity (threshold `dV=0`; matrix noise ~1e-15; mech ~1e-19) |
| **1.0** | CPU/GPU parity (threshold `v` match; matrix noise only; **no mech diffs**) |
| **1.025** | **Not expected to pass yet** — ExpSyn needs Stage 2–3 (device NET_RECEIVE) |

Post-solve / hh segfault / end-of-step mech issues from earlier gpu-native work are
**resolved** on this baseline. Do not re-open “GPU v stuck at −65” as the open bug.

---

## Architecture plan

### Design principles (do not regress)

1. **Full GPU fixed steps** (CoreNEURON-style). No per-step host `vec_rhs` pull →
   `nrn_update_voltage` → push `vec_v` on the hot path.
2. **Heap-free network identity:** `weight_index` only (PR #3826). No
   `NetCon::weight_` heap. No Stage-2 `_receive_weight` buffer shims.
3. **Enqueue on host, apply on device** for NET_RECEIVE (CoreNEURON-shaped).

### Stages

| Stage | Content | Status |
|-------|---------|--------|
| 0 | Revert host SoA mirror after `net_receive` | Done |
| 1 | NetReceiveBuffer registry, enqueue, upload | Done (`1f05cbc19`, still present) |
| **I0** | Merge heap-free + gpu-native → `local/gpu-native-net-soa` | Done (`e0dd78c18`) |
| **I1** | Rebuild + baseline @ 0.025 / 1.0 | Done (2026-07-25) |
| **2** | Codegen: enqueue-only GPU `pnt_receive` + `net_buf_receive` (ExpSyn) using **weight_index** + Weight SoA | **Next** |
| **3** | Wire `deliver_post_step_events` → `update_net_receive_buffer` → registered kernels | Pending |
| **4** | NetStim / net_send buffer path as needed | Pending |
| **5** | Gates: gid 32 @ 1.025 → 688 spikes @ 100 | Pending |

### Stage 2 notes (SoA-aware)

- Host `pnt_receive` when `nt->compute_gpu`: enqueue `(pnt_index, weight_index, flag)` into
  `neuron::gpu::NetReceiveBuffer` — use heap-free index ABI, not `double* - nt->weights`.
- Device kernel: load weight from Weight SoA / device weights via `weight_index`; update
  mechanism SoA (`g += weight` for ExpSyn).
- Register: `hoc_register_net_receive_buffering(net_buf_receive_*, type)`.
- Reference: CoreNEURON `print_net_receive` / `print_net_receive_buffering`;
  NEURON acc already has **net_send** buffering patterns.
- **Do not** revive discarded 2026-07 Stage 2 WIP (`_receive_weight`, prop-row shims).

### Stage 3 notes

After enqueue path works:

1. `update_net_receive_buffer(nt)` (order + device update).
2. Call registered `net_buf_receive` kernels.
3. Clear buffer counts.

Likely hook: after `deliver_post_step_events_host` / `nrn_deliver_events` in lastpart.

---

## Key paths

| Area | Path |
|------|------|
| Fixed step | `src/neuron/gpu/fadvance_gpu.cpp`, `lastpart.cpp` |
| Post-solve | `src/neuron/gpu/post_solve.cpp` |
| Net receive buffer | `src/neuron/gpu/net_receive_buffer.{hpp,cpp}` |
| Net send buffer | `src/neuron/gpu/net_send_buffer.{hpp,cpp}` |
| Event delivery (host) | `src/neuron/gpu/net_events.cpp`, `src/nrncvode/netcvode.cpp` |
| Weight SoA | `src/neuron/container/network/weights.hpp` |
| NMODL NEURON codegen | `src/nmodl/codegen/codegen_neuron_cpp_visitor.cpp` |
| NMODL OpenACC | `src/nmodl/codegen/codegen_neuron_acc_visitor.cpp` |
| Harness | `test/external/ringtest/prcellstate_native_gpu.sh` |

---

## Agent: GPU access (required)

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_NATIVE_GPU_DEVICE_NONVINT=1
export NRN_GPU_BACKEND_TEST=native
export NRN_GPU_PERMUTE=2
```

1. Always use `nrnenv nrngpu build-gpu` (wrong install → missing GPU / `neuron.gpu`).
2. Full shell permissions for harness (GPU/OpenACC/MPI).
3. Verify: `nvidia-smi -L` and `Info : 1 GPUs shared by 1 ranks per node` in `special` output.
4. After major rebuild: `ninja install` then in harness dir `rm -rf x86_64 && nrnivmodl .`

---

## Starting prompt (paste into a new `nrngpu` session)

```
Read ~/neuron/nrngpu/GROK-GPU-NATIVE.md and AGENTS.md.
Also skim doc/network-soa/heap-free.md for weight_index ABI.

Repo: ~/neuron/nrngpu, branch local/gpu-native-net-soa @ e0dd78c18
(+ handoff update if present). Base = heap-free PR #3826 + gpu-native merge.

Goal: pure native GPU fixed step for ringtest with NetReceiveBuffer / NetSendBuffer
on device. Do NOT reintroduce NetCon::weight_ heap or per-step host vec_rhs voltage
update. Do NOT reintroduce Stage-2 _receive_weight shims — use weight_index + Weight SoA.

Baseline verified: prcellstate_native_gpu.sh 32 0.025 and 32 1.0 are clean (noise only).
Next: Stage 2 codegen (enqueue-only pnt_receive + net_buf_receive ExpSyn), then Stage 3
wire-up. Gate: 32 @ 1.025 then 688 spikes @ 100.

Before GPU runs: source ~/neuron/bin/nrnenv nrngpu build-gpu; full shell permissions.
```

---

## Related branches (do not confuse)

| Branch | Role |
|--------|------|
| `local/gpu-native-net-soa` | **Active** — GPU + heap-free |
| `local/cpu-net-soa-heap-free` | PR #3826 source line (base) |
| `local/cpu-network-soa` | Earlier dual-write SoA line |
| `local/gpu-native-qualification` | Pre-merge GPU-only history (merged in) |
