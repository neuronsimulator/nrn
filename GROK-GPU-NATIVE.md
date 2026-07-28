# Grok handoff: NEURON native GPU + heap-free network SoA

Use this file when starting a **new** Grok session rooted in `~/neuron/nrngpu`
on branch **`local/gpu-native-net-soa`**.

Related CPU docs (same tree after merge):

| Doc | Role |
|-----|------|
| `GROK-NETWORK-SOA.md` | Network SoA / dual-write history |
| `doc/network-soa/heap-free.md` | Weight-index ABI, packing A charter |
| `AGENTS.md` | Short agent rules for this branch |
| **`doc/gpu-step-qualification.md`** | Gates A–F; performance / host-traffic guide |

---

## Permanent: high performance is sacred (CoreNEURON is a guide)

**Read this every GPU session.**

**What is sacred:** **high performance** — wall time, device occupancy, and
**minimal host↔device communication on the fixed-step hot path.**

**What is not sacred:** CoreNEURON as a frozen design. It is a **good guide today**
because it already runs full fixed steps on GPU with very little CPU traffic
during a step (mainly spikes + optional trajectories). Its **algorithms are
hopefully improvable**. If you can envision a **better-performing** approach than
CoreNEURON for native GPU, that is a **valid candidate** — measure it, do not
reject it for “not matching CoreNEURON.”

**Practical guide (not a cage):** avoid inventing per-step full node/mech SoA
host mirrors as the primary integration or recording strategy. A high-performance
hot path typically looks like:

1. **Spike / discrete events** (threshold hit indices → host deliver; MPI exchange)
2. **Optional trajectories** (only requested scalars for `Vector.record` / graphs)
3. **End-of-run or diagnostic pull** (`prcellstate`, `finalize_psolve_download`) — not integration

Integration state (V, mechanism SoA, matrix) should stay **device-resident** unless
a measured design needs otherwise.

### Where to look (in-tree CoreNEURON — reference, not law)

| Concern | Path |
|---------|------|
| Fixed-step GPU loop | `src/coreneuron/sim/fadvance_core.cpp` (`nrn_fixed_step_thread`, lastpart) |
| Trajectory send each step | `nrncore2nrn_send_values` in same file — gather requested doubles only |
| Trajectory request ABI (NEURON side) | `src/nrncvode/netcvode.cpp` — search **`trajectory`**: `nrnthread_get_trajectory_requests`, `nrnthread_trajectory_values`, `nrnthread_trajectory_return` |
| Device trajec buffers | `src/coreneuron/gpu/nrn_acc_manager.cpp` (`trajec_requests` copyin) |
| Trajectory struct | `src/coreneuron/sim/multicore.hpp` — `TrajectoryRequests` |

### Trajectory modes (low-traffic recording reference)

From comments on `nrnthread_get_trajectory_requests` / CoreNEURON — a **performant
pattern** for recording without full SoA download:

| Mode | Meaning |
|------|---------|
| **`bsize > 0`** | Device fills trajectory arrays for the interval; hand back at end of stretch |
| **`bsize == 0`** | Per-step sparse gather of `types`/`indices` → host `pvars` / callback |

Better recording schemes are welcome if they move less data or cost less wall time.
**Weak default to avoid:** `if (fixed_record_) sync_state_to_host_for_host_reads()`
every step (full SoA) unless you have a measured reason.

### Implications for native GPU work

- Use CoreNEURON for **ideas and low-traffic shape**, not as a correctness oracle
  for every algorithm choice.
- Spike path already uses device-side buffers (NetReceiveBuffer, NetSendBuffer indices).
- Residual host traffic on the hot path is **performance debt**, not a goal.
  Ringtest psolve no longer transfers V host↔device (deviceptr + no post_solve V pull).
- When stuck on lastpart / record / sync: check CoreNEURON paths above, then ask
  whether a **faster** design is possible.

### Session end: commit, do not push

User workflow (all GPU sessions — and preferred for this tree generally):

1. Finish a coherent step (code + green smokes as appropriate).
2. **Commit locally** with a clear message (complete sentences).
3. **Do not `git push`** unless the user explicitly asks.
4. User reviews with `git show HEAD` (and related), then pushes themselves — or
   discusses and requests **amend** / follow-up commit.

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
| **2** | ACC: enqueue-only GPU `pnt_receive`; `net_buf_receive` via `weight_index` + Weight SoA | **Device apply for all** buffered NET_RECEIVE (incl. net_send → NetSendBuffer + host flush) |
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

1. Full GPU fixed steps with **minimal hot-path host traffic** (see permanent
   performance section). No per-step host `vec_rhs` → host voltage → push `vec_v`
   as the primary fix. Avoid full SoA host mirror as the default recording/lastpart
   strategy unless measured.
2. Heap-free: `weight_index` only; no `NetCon::weight_` heap; no `_receive_weight` shims.
3. Enqueue on host; **device** NET_RECEIVE apply for all buffered point processes.
   `net_send`/`net_move`/`net_event` → device NetSendBuffer (index ABI + heap-free
   `weight_index`); host flush/deliver. Device cur/jacob with PP atomics.
4. Recording: prefer sparse/buffered trajectory-style gather over full SoA pull.

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
ln -sfn ~/models/82894/mod/*.mod .
nrnivmodl -nmodl "$(which nmodl)" \
  -nmodlflags "passes --inline host --c acc --oacc" .
# special: /tmp/traub-nrngpu-acc/x86_64/special
# Do NOT set NRN_GPU_ALLOW_UNQUALIFIED for certified runs.
```

#### Fixes landed for ACC Traub

1. **Header install:** ship `neuron/model_data.hpp` + transitive container/network
   headers and `utils/logger.hpp` (`cmake/NeuronFileLists.cmake`).
   **Verified 2026-07-26 (A2):** delete from install → `ninja install` restores from
   staging; clean Traub ACC `nrnivmodl` succeeds with only install `-I` (no
   source-tree hand-copy).
2. **ACC INITIAL + wrote_conc:** host-only init must not use `_present_fp_*`
   (`print_nrn_init` + `indexed_fp_var` honor `use_present_fp_indexing_`).
3. **ACC net_buf_receive + net_send:** define `_ppvar` / `_pnt` / `_weight_index`
   in the host apply loop so PulseSyn/NMDA self-events compile.
4. **Linkage (prior):** `extern "C"` for net send/receive buffering registration.

**Known residual:** M3 NMODL feature gaps stay deferred. Process-exit
`acc_delete` noise mitigated by `finalize_device_resources()` in
`hoc_final_exit` (2026-07-26); see architecture debt below for the longer-term
fix. AlphaSynKin ACC eigen functor fixed (A3, 2026-07-26): functors use
`_present_fp_*` indexing.

### Architecture debt: single device-resource owner (not near-term)

**Not** about per-step voltage traffic. “Threshold mirrors” are **metadata**
columns for Gate E detect (`h_thvar_row`, `h_threshold`, `h_flag` — one row per
threshold PreSyn), OpenACC-`copyin`’d for the detect kernel. Detect reads
**device `vec_v` in place** (Th1/Th2). Per-step host traffic is only **hysteresis
flags + hit slot indices** when there are crossings — **not** a full voltage
pull (Th2).

**Problem:** device lifetime is split:

| Resource | Owner today |
|----------|-------------|
| Node/mech/Weight SoA | `UploadState` via `device_token` / last `model_sorted_token` |
| Threshold columns + some `net_send` buffers | File-static `g_tables` in `check_thresh.cpp` (auxiliaries) |

Auxiliaries were freed on Model “unsorted” / exit **after** OpenACC atexit could
already deinitialize CUDA → `acc_delete` Deinitialized. Near-term fix:
`finalize_device_resources()` while the device is still live + late no-ops.

**Right architectural fix (return later):** one **device-resource domain** for
all OpenACC mirrors (SoA upload + threshold columns + net_send buffers + any
future aux), with:

1. Explicit upload/invalidate tied to layout tokens (not file-statics).
2. Single `finalize` at session end (hoc exit), not static-destructor order.
3. No `acc_delete` from Model unsorted callbacks after finalize.

Do **not** reintroduce full `vec_v` host pull as part of that cleanup.

---

## Device NET_RECEIVE apply (2026-07-26) — **landed on integration tip**

| Item | Value |
|------|--------|
| Integration branch | `local/gpu-native-net-soa` @ `9080f3832` (FF from `local/gpu-device-net-receive`, pushed) |
| Commits | `c9f85c84a` device apply + index NetSendBuffer; `9080f3832` Traub spike-timing fix |
| Device NET_RECEIVE | Buffered PP: OpenACC apply of body (Weight SoA + `_present_fp_*`); no host body apply |
| net_send path | Device buffers **indices**; host deliver via `ml->pdata` + heap-free `weight_index` |
| Ringtest 1.025 / 100 | **GREEN** (688 @ 100; noise-level cellstate) — re-checked 2026-07-26 |
| Traub 1/10 no-gap | **GREEN** re-smoke 2026-07-26: QUALIFIED A–E; **4474** @ 100; clean exit (rebuild mechs after ABI change) |

```bash
# After libnrniv changes that touch NetSendBuffer symbols: rebuild Traub special
rm -rf /tmp/traub-acc && mkdir -p /tmp/traub-acc && cd /tmp/traub-acc
ln -sfn ~/models/82894/mod/*.mod .
nrnivmodl -nmodl "$(which nmodl)" -nmodlflags "passes --inline host --c acc --oacc" .
# Do not reuse a special linked against an older libnrniv (symbol lookup errors).
```

### Gate F + lastpart SoA / V host traffic (2026-07-26)

| Item | Status |
|------|--------|
| Post-nonvint full SoA pull | **Gated** — only if AFTER_SOLVE / BEFORE_STEP / `Vector.record` |
| Pre-nonvint full SoA pull | **Removed** |
| Pre-nonvint V pull | **Removed** — wait only before device nonvint |
| Host V during psolve | **None on ringtest** — device owns `vec_v`; ACC/treeset/post_solve/thresh use `deviceptr`, not `present(host V)` |
| Ringtest Gate F | **yes** |
| Ringtest 0.025/1/1.025/100 | **GREEN** (688 spikes; noise-level cellstate) with **no host↔device V** on the hot path |

**Host V residual (closed for ringtest psolve):** OpenACC `present(node_voltages)`
was re-binding / re-entering host V onto the device; poison host V → device dumps
showed 1e300. Fix: codegen `_d_voltages = acc_deviceptr(...)` + `deviceptr(_d_voltages)`
in CURRENT/STATE/init; same for axial rhs, voltage update, and Th1 detect.
Explicit `sync_node_voltages_to_device` remains only for VecPlay / host-post-solve
fallbacks (not ringtest). Host still pulls V when Th2/WATCH or dump needs it.

**Trajectory for record** still open (Gate F stays red when `Vector.record` forces full SoA).

### NetSendBuffer capacity (no silent drop)

Device kernels **cannot** grow mid-region. Host pre-sizes via
`net_send_buffer_ensure_for_events(ml, min_events)`:

- Default capacity: `max(1024, nodecount × HEADROOM)` (HEADROOM default **4**)
- Per flush: `max(default, min_events × HEADROOM, 2 × high_water)`
- After each GPU pull: `record_peak` adapts for the next kernel; re-copyin if grown
- Overflow: **abort** with cnt/size (never continue with dropped events)
- Env override: `NRN_GPU_NET_SEND_BUFFER_HEADROOM` (1–64)
- Threshold hit list: sized to slot count; overflow also aborts (no partial deliver)

### Traub 1/10 no-gap raster parity (2026-07-27)

Goal: **exact** CPU vs GPU spike multiset `(t, gid)` — not only count 4474.

| Rev / special | Result (vs same CPU oracle, 4474 spikes) |
|---------------|------------------------------------------|
| **`9080f3832`** (device NET_RECEIVE land; host V still mirrored) | **Not exact** in re-smoke: 4 × **+1 dt** late spikes (gid **313** only). Portfolio “exact raster” claim is **not reliable**. |
| **Tip** `local/gpu-lastpart-no-soa-pull` | Count still **4474**, but **not exact**; typically ~8–45 exclusive spikes, almost all **±1 dt** on a few gids. |
| Gate F first (`8fae4e29f`) | Much worse when measured (~37 exclusives) — early Gate F before deviceptr. |
| Force full post-nonvint SoA pull (experiment) | **Worse**, not better — do not reintroduce full SoA pull as the Traub fix. |

**Nondeterminism (critical):** same tip binary, 5 GPU runs → exclusive-spike counts **8, 20, 33, 37, 45** (run-to-run). Exact match was observed once on a re-run then not again. CPU oracle is stable (`9080` CPU == tip CPU).

**prcellstate at first tip miss (gid 130, t=25.2):** threshold **V** noise-level (`dV ~ 3e-11`); matrix noise; **NMDA `A` (field 7)** large relative diffs (~0.8). Path looks like **NMDA ramp / net_send / device NET_RECEIVE** accumulation, not host `vec_rhs` voltage hot path or host NET_RECEIVE body.

**Codegen fix landed:** host-only INITIAL (`wrote_conc`, e.g. `cad`) must not use undeclared `_d_voltages` (broke fresh Traub ACC rebuild after deviceptr commit).

**Deterministic testing flags** (`src/neuron/event_order.hpp`, all four backends):

| Env | Effect |
|-----|--------|
| `NRN_DETERMINISTIC_EVENTS=1` | Total-order GPU→host event flushes (hits, NetSend, NRB) |
| `NRN_DETERMINISTIC_MATRIX=1` | PP `nrn_cur`/`nrn_jacob`: `acc loop seq` by instance id (no unordered atomics on `vec_rhs`/`vec_d`) |

Matrix flag is slow on GPU (~12 min Traub 100 ms vs ~50 s); testing only.
Dual-build Traub: `nrnivmodl -nmodl … -nmodlflags "… acc …" -coreneuron`.

**Traub 1/10 @100 measure (2026-07-27, dual special):** oracle = NEURON CPU 4474.

| Backend | none | events | matrix | both |
|---------|------|--------|--------|------|
| NEURON CPU | exact | exact | exact | exact |
| CoreNEURON CPU | exact | exact | exact | exact |
| CoreNEURON GPU | exact, stable | exact, stable | exact, stable | exact, stable |
| NEURON native GPU | miss~21, nondet | miss~12–31, nondet | miss 0–6, nondet* | miss 4–8, nondet |

\*matrix run1 once exact; run0≠run1. Det flags reduce miss count but do not freeze native GPU.

**Landed (2026-07-27):** Traub 1/10 no-gap **exact** CPU/GPU raster multiset (4474)
and run-to-run GPU equality after NMDA self-event path fix:
- device `net_send_buffering` **always** atomic capture on `_cnt` (stale device
  `nt->compute_gpu==0` used the non-atomic branch → races dropped ~all net_sends;
  ~300 of ~1e5 NMDA stimuli scheduled self-events before fix);
- `update device(nt.compute_gpu)` when entering fixed-step GPU;
- net_buf `net_send` uses event `t` (`nrb->_nrb_t`), not restored `nt->_t`.
prcellstate t=10 gid321: NMDA A/B/k exact CPU↔GPU.

**Next (this line):** trajectory native path / residual noise (ca/cad ~1e-10); optional
det-event keep for testing. Not host NET_RECEIVE / not host vec_rhs voltage hot path.

### Next GPU feature (new session)

| Option | Content |
|--------|---------|
| **Traub exact raster** | **Done** — 4474 exact multiset CPU/GPU + GPU/GPU |
| Trajectory native path | Low-traffic record so Gate F stays green with `Vector.record` |
| Later | use_gap=1, multi-rank, perf, single device-resource owner |

### Constraints (do not regress)

1. Full GPU fixed steps; minimize host traffic on the hot path (performance first).  
2. Heap-free: `weight_index` only.  
3. Ringtest long gate and Traub QUALIFIED bar stay green if you touch shared paths.  
4. **No host body apply** of NET_RECEIVE on the native-GPU path (host **deliver** of buffered net_send is OK).  
5. Do not “fix” recording or lastpart with unmeasured full SoA pull as the long-term design.  
6. **Commit without push** at end of a step; user reviews `git show HEAD`.

---

## Key paths

| Area | Path |
|------|------|
| **CoreNEURON guide (step)** | `src/coreneuron/sim/fadvance_core.cpp` |
| **Trajectory ABI (NEURON)** | `src/nrncvode/netcvode.cpp` (`trajectory` search) |
| Event flush | `src/neuron/gpu/net_events.cpp` |
| NetReceiveBuffer | `src/neuron/gpu/net_receive_buffer.{hpp,cpp}` |
| Weight SoA upload | `src/neuron/gpu/upload.cpp` |
| ACC codegen (atomic PP) | `src/nmodl/codegen/codegen_neuron_acc_visitor.cpp` |
| Threshold detect (Th0–Th3) | `doc/gpu/threshold-detection.md`, `src/neuron/gpu/check_thresh.*` |
| Gate F lastpart | `src/neuron/gpu/lastpart.cpp` |
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
Read ~/neuron/notes/PORTFOLIO.md (GPU-native) then GROK-GPU-NATIVE.md and AGENTS.md.
High performance is sacred; CoreNEURON is a guide (low host traffic), not law.
Commit steps locally without push unless asked.

Tree: ~/neuron/nrngpu. Kind: feature.
Branch: local/gpu-lastpart-no-soa-pull — Gate F + no host↔device V during psolve
+ NetSendBuffer capacity. Ringtest 688@100. Traub count 4474 but raster not exact
and GPU runs are nondeterministic (±1 dt on a few gids; NMDA A drifts).

Next: Traub 1/10 no-gap **exact** CPU/GPU raster (determinism first).
Do not start use_gap, multi-rank, or device-resource owner unless asked.

Heap-free weight_index only; no host NET_RECEIVE body; no host vec_rhs voltage hot path.
source ~/neuron/bin/nrnenv nrngpu build-gpu before GPU runs.
```

---

## Related branches

| Branch | Role |
|--------|------|
| `local/gpu-native-net-soa` | **Integration tip** — Th0–Th4 + device NET_RECEIVE |
| `local/gpu-device-net-receive` | Feature history (FF into tip) |
| `local/cpu-net-soa-heap-free` | PR #3826 base |
