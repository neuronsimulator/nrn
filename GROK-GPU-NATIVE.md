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

## Canonical repo and branch

| Item | Value |
|------|--------|
| Repo | `~/neuron/nrngpu` (primary; commit here) |
| Branch | **`local/gpu-native-net-soa`** |
| Base | `local/cpu-net-soa-heap-free` (PR [#3826](https://github.com/neuronsimulator/nrn/pull/3826)) + GPU overlay |
| Build / install | `~/neuron/nrngpu/build-gpu` via `nrnenv` / `ninja` / `ninja install` |
| Dev doc (recipe) | `docs/dev/native-gpu-build.rst` (CMake, activate, NMODL ACC, smokes) |

---

## Qualification goal

**HH GPU-native ringtest:** 688 spikes @ `tstop=100` with CPU parity.

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_NATIVE_GPU_DEVICE_NONVINT=1
export NRN_GPU_BACKEND_TEST=native
export NRN_GPU_PERMUTE=2

cd ~/neuron/nrngpu/build-gpu/test/external_ringtest/neuron_gpu_native_mpi
# After install if needed: rm -rf x86_64 && nrnivmodl .

./prcellstate_native_gpu.sh 32 0.025 0
./prcellstate_native_gpu.sh 32 1 0
./prcellstate_native_gpu.sh 32 1.025 0   # first NetCon — GREEN
./prcellstate_native_gpu.sh 32 100       # long gate — GREEN (688 spikes)
```

---

## Baseline (2026-07-25, after Stage 3c)

| tstop | Result |
|-------|--------|
| **0.025** | `dV=0`, noise ~1e-15 |
| **1.0** | `dV=0` |
| **1.025** | **GREEN** — `dV=0`; post_setup rhs/d exact match (no host matrix augment) |
| **100** | **GREEN** (2026-07-25) — **688 spikes** both sides; threshold `dV=0`; max \|d\| ~1e-13 (noise) |

### Stages 2–3c (done)

| Stage | Content | Status |
|-------|---------|--------|
| **2** | ACC: enqueue-only GPU `pnt_receive`; `net_buf_receive` via `weight_index` + Weight SoA | Done (host apply of NET_RECEIVE body + device float update) |
| **3** | Flush NetReceiveBuffer after `deliver_net_events` **and** post-step deliver | Done |
| **3b** | Interim host matrix augment for net_buf mechs | Superseded by 3c |
| **3c** | Root cause: multi-PP atomic matrix updates; remove host augment | **Done** |

### Stage 3c root cause

Ringtest places **two ExpSyn on the same node**. OpenACC `nrn_cur` / `nrn_jacob` loop over instances in parallel with non-atomic:

```cpp
vec_rhs[node_id] -= rhs;
vec_d[node_id] += g_unused;
```

Race: zero-contrib instance can overwrite the non-zero synaptic update → exact missing
Δrhs ≈ 0.103 and Δd ≈ `g_unused` at post_setup while mech SoA (`g`/`i`/`g_unused`) still matched
(serial field writes per instance, no cross-instance share).

**Fix:** emit `nrn_pragma_acc(atomic update)` (and OMP atomic) before `vec_rhs` /
`vec_d` updates when `info.point_process` (ACC visitor). Density mechs remain
one-instance-per-node and need no atomics.

### Event timing note

NetStim `start=0`, delay=1 → first events at **t=1.0**, delivered at **start** of the
step ending at 1.025 via `deliver_net_events` (`til = t + 0.5*dt`). Both delivery
points must flush NetReceiveBuffer.

### Design constraints (do not regress)

1. Full GPU fixed steps — no per-step host `vec_rhs` → host voltage → push `vec_v`.
2. Heap-free: `weight_index` only; no `NetCon::weight_` heap; no `_receive_weight` shims.
3. Enqueue on host; prefer device apply. Current: host NET_RECEIVE apply into SoA +
   device cur/jacob with atomics for PP matrix.

### Threshold detection (Th0+)

| Stage | Content | Status |
|-------|---------|--------|
| **Th0** | Contract: slot table = sole detect set; hit buffer = slot indices; host deliver; no InputPreSyn | **Done** — `doc/gpu/threshold-detection.md` |
| **Th1** | OpenACC detect over slots (device `vec_v`, atomic hit buffer) | **Done** |
| **Th2** | Skip full `vec_v` host pull when device detect handles SoA PreSyns (lazy pull for host/WATCH) | **Done** |
| **Th3** | Re-qualify ringtest 0.025/1/1.025/100 after Th0–Th2; traffic notes | **Done** (2026-07-25) |
| **Th4** | Traub-scale threshold load | **Green** (2026-07-25) — QUALIFIED yes; 4474 spikes @ 100; noise-level prcellstate |

Today: Th1 device `pscheck` + Th2 (no forced host voltage sync before detect) +
Th3 green long gate. Host still pulls flags/hit indices then
`deliver_threshold_spike`.

### Resume: Traub / Th4 (2026-07-25+)

NMODL **CPU** Traub M0–M2 is green in `~/neuron/nrnnmodl`
(`local/nmodl-cpu-traub`; see `GROK-NMODL-CPU.md`). Do not re-do M0–M2 there.

**#3826 tip absorbed** via cherry-pick (full rebase of merge-heavy GPU history
conflicted on unrelated commits):

| Cherry-pick | Content |
|-------------|---------|
| `3b273d4e3` (from `dcdbf97fe`) | Weight SoA by index (drop reverse NetCon map) — Traub hang fix |
| `c267a0788` (from `a49fbad57`) | `nrnivmodl` defaults for `NMODL_PYLIB` / `NMODLHOME` |

**Progress after absorb:**

| Step | Status |
|------|--------|
| Rebuild + ringtest 688 @ 100 | **Green** (noise-level cellstate) |
| NMODL Traub mechs (host `--neuron`) | **Green** — load; CPU 4474 @ 100 |
| OpenACC Traub mechs + Gate B/C | **Green** — QUALIFIED yes (A–E) |
| Th4 GPU one-step / multi-step | **Green** — gid 171 prcellstate noise-level @ 0.025 and 1.0 |
| Th4 GPU spikes | **Green** — **4474** @ 100 ms exact vs CPU |

#### Traub NMODL OpenACC build (Gate B/C path)

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_NATIVE_GPU_DEVICE_NONVINT=1 NRN_GPU_BACKEND_TEST=native NRN_GPU_PERMUTE=2
# Install must ship neuron/model_data.hpp + container headers (cmake/NeuronFileLists.cmake).
mkdir -p /tmp/traub-nrngpu-acc && cd /tmp/traub-nrngpu-acc
# Skip unused AlphaSynKin* (ACC eigen-functor codegen bug: missing _lmc member).
for m in ~/models/82894/mod/*.mod; do
  case $(basename "$m") in alphasynkin.mod|alphasynkint.mod) ;; *) ln -sfn "$m" .;; esac
done
nrnivmodl -nmodl "$(which nmodl)" \
  -nmodlflags "passes --inline host --c acc --oacc" .
# special: /tmp/traub-nrngpu-acc/x86_64/special
# Do NOT set NRN_GPU_ALLOW_UNQUALIFIED for certified runs.
```

#### Fixes landed for ACC Traub

1. **Header install:** ship `neuron/model_data.hpp` + transitive container/network
   headers and `utils/logger.hpp` (`cmake/NeuronFileLists.cmake`).
2. **ACC INITIAL + wrote_conc:** host-only init must not use `_present_fp_*`
   (`print_nrn_init` + `indexed_fp_var` honor `use_present_fp_indexing_`).
3. **ACC net_buf_receive + net_send:** define `_ppvar` / `_pnt` / `_weight_index`
   in the host apply loop so PulseSyn/NMDA self-events compile.
4. **Linkage (prior):** `extern "C"` for net send/receive buffering registration.

**Known residual:** process-exit OpenACC `acc_delete` / `cuDeviceGetAttribute`
deinitialized (post-run cleanup noise; does not affect results). AlphaSynKin
ACC eigen functor still broken (unused in 1/10 Traub). M3 NMODL feature gaps
stay deferred.

---

## Key paths

| Area | Path |
|------|------|
| Event flush | `src/neuron/gpu/net_events.cpp` |
| NetReceiveBuffer | `src/neuron/gpu/net_receive_buffer.{hpp,cpp}` |
| Weight SoA upload | `src/neuron/gpu/upload.cpp` |
| ACC codegen (atomic PP) | `src/nmodl/codegen/codegen_neuron_acc_visitor.cpp` |
| Threshold detect (Th0–Th3) | `doc/gpu/threshold-detection.md`, `src/neuron/gpu/check_thresh.*` |
| Harness | `test/external/ringtest/prcellstate_native_gpu.sh` |

---

## Agent: GPU access

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_NATIVE_GPU_DEVICE_NONVINT=1
export NRN_GPU_BACKEND_TEST=native
export NRN_GPU_PERMUTE=2
```

After ACC codegen changes to built-ins: `rm -f build-gpu/src/nrnoc/expsyn.cpp && ninja install`.

---

## Starting prompt

```
Read GROK-GPU-NATIVE.md and AGENTS.md.

Stages 2–3c + threshold Th0–Th3 done; long gate green: 688 spikes @ tstop=100.
NMODL CPU Traub M0–M2 green in ~/neuron/nrnnmodl — do not re-do there.

#3826 tip absorbed; ringtest 688@100 green. Traub NMODL OpenACC QUALIFIED yes
(Gates A–E); Th4 green: 4474 spikes @ 100, prcellstate noise-level @ 0.025/1.

Do not reintroduce NetCon::weight_ heap or host vec_rhs voltage hot path.
source ~/neuron/bin/nrnenv nrngpu build-gpu before GPU runs.
```

---

## Related branches

| Branch | Role |
|--------|------|
| `local/gpu-native-net-soa` | **Active** |
| `local/cpu-net-soa-heap-free` | PR #3826 base |
