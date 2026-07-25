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

./prcellstate_native_gpu.sh 32 0.025 0
./prcellstate_native_gpu.sh 32 1 0
./prcellstate_native_gpu.sh 32 1.025 0   # first NetCon event — GREEN
./prcellstate_native_gpu.sh 32 100       # long gate (688 spikes)
```

---

## Baseline after Stage 3b (2026-07-25)

| tstop | Result |
|-------|--------|
| **0.025** | CPU/GPU parity (`dV=0`; noise ~1e-15) |
| **1.0** | CPU/GPU parity (`dV=0`; matrix noise only) |
| **1.025** | **GREEN** — `dV=0`; max \|d\| ~2e-15 |
| **5** | Smoke OK — `dV~1e-13`; 32 spikes both sides |
| **100** | Not re-run this session — next long gate |

### Stage 2–3–3b summary

1. **Stage 2:** ACC codegen enqueue-only `pnt_receive` when `compute_gpu`;
   `net_buf_receive` applies NET_RECEIVE via `weight_index` + Weight SoA
   (host apply + `update device` of mech floats). Register with
   `hoc_register_net_receive_buffering`.
2. **Stage 3:** Flush NetReceiveBuffer after **both**
   `deliver_net_events_host` (start, `til=t+0.5*dt` — first events at t=1.0)
   and `deliver_post_step_events_host` (end, `til=t`).
3. **Stage 3b:** Device OpenACC CURRENT/JACOBIAN for ExpSyn updated mech SoA
   (`g`/`i`/`g_unused`) but **did not write** synaptic terms into device
   `vec_rhs`/`vec_d` (post_setup dump: exact missing Δrhs≈0.103, Δd≈g_unused).
   Fix: `augment_device_matrix_for_net_receive_mechs` after `nrn_lhs` —
   pull device matrix, re-run host cur/jacob for `net_buf_receive` types only,
   push matrix back for the device solver.

### Design constraints (do not regress)

1. **Full GPU fixed steps.** No per-step host `vec_rhs` pull → host voltage update
   → push `vec_v` as the primary fix.
2. **Heap-free network identity:** `weight_index` only. No `NetCon::weight_` heap.
   No Stage-2 `_receive_weight` shims.
3. **Enqueue on host, apply on device** long-term. Current interim: host apply of
   NET_RECEIVE into Weight/mech SoA + Stage 3b host matrix augment for net-receive
   mechs. Prefer pure device matrix writes once the OpenACC PP cur/jacob issue is
   understood.

---

## Architecture plan

| Stage | Content | Status |
|-------|---------|--------|
| 0–1 | NetReceiveBuffer infrastructure | Done |
| I0–I1 | Merge + baseline 0.025 / 1.0 | Done |
| **2** | Codegen enqueue + net_buf_receive (weight_index) | **Done** (host apply interim) |
| **3** | Wire deliver_net_events + post_step flush | **Done** |
| **3b** | Device matrix gets ExpSyn after NET_RECEIVE | **Done** (host augment interim) |
| **3c** | Root-cause OpenACC PP vec_rhs/vec_d writes; drop host augment | Pending |
| **4** | NetStim / net_send as needed | Pending |
| **5** | 688 spikes @ 100 | **Next** |

### Key insight (event timing)

NetStim `start=0`, NetCon `delay=1` → first events at **t=1.0**, delivered at
**start** of the step ending at 1.025 via `deliver_net_events` (`til=t+0.5*dt`),
not only at end-of-step. Both delivery points must flush NetReceiveBuffer.

### Open follow-ups

- **OpenACC bug:** Why ExpSyn device cur writes `i`/`g_unused` correctly but not
  `vec_rhs[node]` / `vec_d[node]` (density mechs write matrix fine). Investigate
  present/atomic for point-process cur; then remove
  `augment_device_matrix_for_net_receive_mechs`.
- Prop is edit-epoch only for GPU; sim path uses indices. No Prop-removal blocker.
- Long gate: `./prcellstate_native_gpu.sh 32 100` → 688 spikes + rdcellstate.

---

## Key paths

| Area | Path |
|------|------|
| Event flush + Stage 3 | `src/neuron/gpu/net_events.cpp` |
| NetReceiveBuffer + Stage 3b augment | `src/neuron/gpu/net_receive_buffer.{hpp,cpp}` |
| setup_tree_matrix hook | `src/nrnoc/treeset.cpp` |
| Weight SoA upload | `src/neuron/gpu/upload.cpp` |
| ACC codegen | `src/nmodl/codegen/codegen_neuron_acc_visitor.cpp` |
| Harness | `test/external/ringtest/prcellstate_native_gpu.sh` |

---

## Agent: GPU access (required)

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_NATIVE_GPU_DEVICE_NONVINT=1
export NRN_GPU_BACKEND_TEST=native
export NRN_GPU_PERMUTE=2
```

1. Always use `nrnenv nrngpu build-gpu`.
2. Full shell permissions for harness (GPU/OpenACC/MPI).
3. Verify: `nvidia-smi -L` and `Info : 1 GPUs shared by 1 ranks per node`.
4. After rebuild: `ninja install`; if ACC codegen changed built-ins:
   `rm -f build-gpu/src/nrnoc/expsyn.cpp && ninja install`.
5. Harness: `rm -rf x86_64 && nrnivmodl .` after major install when needed.

---

## Starting prompt

```
Read ~/neuron/nrngpu/GROK-GPU-NATIVE.md and AGENTS.md.

Repo: ~/neuron/nrngpu, branch local/gpu-native-net-soa.
Stages 2–3b done: enqueue NET_RECEIVE, flush both delivery points, host matrix
augment for net_buf mechs. Gates 0.025 / 1.0 / 1.025 green (noise only).

Next: prcellstate_native_gpu.sh 32 100 (688 spikes), then optional Stage 3c
(root-cause OpenACC PP matrix writes; drop host augment).

Do NOT reintroduce NetCon::weight_ heap or per-step host vec_rhs voltage update.
Before GPU: source ~/neuron/bin/nrnenv nrngpu build-gpu.
```

---

## Related branches

| Branch | Role |
|--------|------|
| `local/gpu-native-net-soa` | **Active** — GPU + heap-free |
| `local/cpu-net-soa-heap-free` | PR #3826 source line |
| `local/gpu-native-qualification` | Pre-merge GPU history (merged in) |
