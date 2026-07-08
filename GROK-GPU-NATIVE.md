# Grok handoff: NEURON native GPU qualification (`nrngpu`)

Use this file when starting a **new** Grok session rooted in `~/neuron/nrngpu`.
Do **not** resume the old `~/neuron/core-neuron-gpu` session if you want the
workspace path, defaults, and `AGENTS.md` discovery to match this repo.

---

## Why the UI still shows `core-neuron-gpu`

Grok sessions are stored under `~/.grok/sessions/<encoded-cwd>/<session-id>/`.
The **working directory is fixed when the session is created**. `/resume` reloads
that session’s history and keeps its original workspace label — changing your
shell `cd` or “resuming” does not re-bind the session to a new folder.

To work in `nrngpu`:

1. Open the folder as the project root (Cursor: **Open Folder → `~/neuron/nrngpu`**).
2. Start a **new** session (`/new`, or quit and launch Grok from `~/neuron/nrngpu`).
3. Do **not** pick **Resume** on the old `core-neuron-gpu` session.
4. Paste the **starting prompt** below (or: “Read `GROK-GPU-NATIVE.md` and continue”).

Optional: `cd ~/neuron/nrngpu && grok` (no `--resume <old-id>`) starts fresh for
this directory.

---

## Canonical repo and branch

| Item | Value |
|------|--------|
| Repo | `~/neuron/nrngpu` (primary; commit here) |
| Branch | `local/gpu-native-qualification` |
| HEAD (2026-07-07) | `6a5aa1aff` — hh OpenACC segfault fix (`make_instance` host global pointers) |
| Prior commit | `1f05cbc19` — Stage 1 NetReceiveBuffer plumbing |
| Build / install | `~/neuron/nrngpu/build-gpu` via `./grok-bld configure\|build\|test` or `nrnenv` |
| Read-only salvage | `~/neuron/core-neuron-gpu` (no `.git`; notes/patches only) |

---

## Qualification goal

**HH GPU-native ringtest:** 688 spikes @ `tstop=100` with CPU parity.

Harness:

```bash
cd ~/neuron/nrngpu/build-gpu/test/external_ringtest/neuron_gpu_native_mpi
./prcellstate_native_gpu.sh 32 1          # quick prcellstate @ 1 ms
./prcellstate_native_gpu.sh 32 1.025 0    # first NetCon event (no phase dumps)
./prcellstate_native_gpu.sh 32 1.025 1    # same, with last-step phase dumps
./prcellstate_native_gpu.sh 32 100        # long gate (688 spikes)
```

Source (committed): `test/external/ringtest/prcellstate_native_gpu.sh` — copied into the
build tree at configure time.

Compare:

```bash
python ~/models/82894/rdcellstate.py --ignore-unused \
  32_-t1.nrndat 32_-gpu-t1.nrndat
```

---

## Architecture plan (user-approved)

| Stage | Content | Status |
|-------|---------|--------|
| 0 | Revert host SOA mirror after `net_receive` | Done (`0301ffabc`) |
| 1 | NetReceiveBuffer registry, enqueue, upload | Done (`1f05cbc19`) |
| 2 | Codegen: enqueue-only `pnt_receive` + GPU `net_buf_receive` (ExpSyn) | Pending |
| 3 | Wire `nrn_deliver_events` → receive buffer → kernels | Pending |
| 4–6 | NetStim send, full ExpSyn GPU path, gid 32 @ 1.025 → 688 @ 100 | Pending |

**Design principle (do not regress):** CoreNEURON-style **full GPU fixed steps**
for this test — **no per-step CPU↔GPU mixed post-solve** (no pull `vec_rhs` to
host for voltage update on the hot path). Before 1 ms there are **no spikes**;
event delivery is not the first parity blocker.

---

## Current open bug (after segfault fix)

Segfault in `nrn_state_hh` is fixed (`6a5aa1aff`: `inst.global = &hh_global`, not
device pointer in `make_instance`).

**Remaining @ `tstop=1`, gid 32:**

- CPU `v` ≈ -64.978; GPU `v` = -65.0 (initial), GPU `rhs` = 0 on host dump
- Matrix/mech small diffs; HH STATE ran on GPU using stale device `v`
- Likely: **device `post_solve` not applying** `vec_v += rhs` after **CUDA**
  `solve_interleaved2` (OpenACC/CUDA coherence on `vec_rhs`), not missing prcellstate sync alone
- **Rejected approach:** `post_solve_host_after_device_solver` (host pull + push) — user explicitly declined

**Investigate next (device-only):**

- `post_solve_on_device` / `update_voltage_on_device` after CUDA solve
- `use_cuda_launcher()` path in `cellorder.cpp` vs OpenACC `present(vec_rhs)`
- End-of-run download (`finalize_psolve_download`, `prcellstate` sync) — secondary; device state wrong during run

Key paths:

- `src/neuron/gpu/post_solve.cpp`, `fadvance_gpu.cpp`
- `src/coreneuron/permute/cellorder.cpp` (CUDA launcher)
- `src/nrniv/prcellstate.cpp`, `src/neuron/gpu/download.cpp`
- Regressions vs `dbc711808` / `098121133` / `4d01ad064` (sync removals in `fadvance.cpp`)

---

## Agent: GPU access (required)

The agent **can** use the T1000 on this machine when the shell is set up correctly.

**Always before GPU commands:**

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_NATIVE_GPU_DEVICE_NONVINT=1
export NRN_GPU_BACKEND_TEST=native
export NRN_GPU_PERMUTE=2
```

**Run tests from:**

```bash
cd ~/neuron/nrngpu/build-gpu/test/external_ringtest/neuron_gpu_native_mpi
```

**Agent rules:**

1. Use `nrnenv nrngpu build-gpu` — never assume `~/neuron/nrngpu/build-gpu/install` on `PATH` without it (wrong install → “no GPU” / missing `neuron.gpu`).
2. Request **full shell permissions** when the harness needs GPU/OpenACC/MPI.
3. Verify GPU: `nvidia-smi -L` then a short `-gpu-native -tstop 0.025` run; expect `Info : 1 GPUs shared by 1 ranks per node`.
4. Workspace folder at top of UI is cosmetic for **resumed** old sessions; cwd + `AGENTS.md` matter for new work.

**Rebuild after codegen / `hh.cpp` changes:**

```bash
cd ~/neuron/nrngpu/build-gpu && ninja nrniv && ninja install
# ringtest special in test dir may need regen via ctest/external build
```

---

## Starting prompt (paste into a new `nrngpu` session)

```
Read ~/neuron/nrngpu/GROK-GPU-NATIVE.md and ~/neuron/nrngpu/AGENTS.md first.

Repo: ~/neuron/nrngpu, branch local/gpu-native-qualification @ 6a5aa1aff.

Continue native GPU ringtest qualification. Segfault fix is committed. Do NOT
reintroduce mixed CPU/GPU per-step post-solve (no host vec_rhs pull for voltage
update on the hot path). Goal: full GPU fixed step like CoreNEURON.

Open bug: @ prcellstate_native_gpu.sh 32 1, GPU v stays -65, rhs 0 vs CPU ~-64.978. Diagnose and
fix device-only post_solve after CUDA solve_interleaved2.

Before any GPU run: source ~/neuron/bin/nrnenv nrngpu build-gpu and use full
shell permissions. Run prcellstate_native_gpu.sh 32 1 and rdcellstate to verify.

Stage 2 (ExpSyn net_buf_receive codegen) waits until v/matrix parity @ t=1.
```

---

## Old session transcript (optional)

If Grok did not carry full history into the new session, the prior conversation
may be in:

`~/.grok/sessions/%2Fhome%2Fhines%2Fneuron%2Fcore-neuron-gpu/<session-id>/updates.jsonl`

Search that file for `rdcellstate`, `post_solve`, `NetReceiveBuffer`, `6a5aa1aff`.