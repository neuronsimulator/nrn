# GPU-native PreSyn threshold detection

**Status:** Th0–**Th3** done. Device detect over slot columns + device `vec_v`;
host pull of full `vec_v` only when host PreSyn::check or WATCH still need it.
Spike deliver remains host-only. Ringtest 100 re-qualified after Th2.

**Related:** `src/neuron/gpu/check_thresh.{hpp,cpp}`,
`NetCvode::check_thresh` in `src/nrncvode/netcvode.cpp`,
CoreNEURON `NetCvode::check_thresh` in `src/coreneuron/network/netcvode.cpp`.

---

## 1. Goal

Adopt CoreNEURON’s **GPU PreSyn threshold detection** shape for NEURON
gpu-native:

1. Compare voltage to threshold **without requiring a host `PreSyn` walk**.
2. Record hits in a compact **integer buffer**.
3. **Deliver** spikes only on the host (`PreSyn::send` / NetCon / MPI).

Long-term: leave voltages on device for the detect step (no full `vec_v` pull
just for threshold). Th1 implements the device loop; Th0 freezes *who* is
detected and *what* the buffer means.

**“Threshold mirrors” (implementation term):** device copies of the **slot
metadata** columns (`thvar_row`, `threshold`, hysteresis `flag`) used by the
OpenACC detect kernel. They are **not** a per-step voltage mirror. Detect
indexes **device `vec_v`** already resident for the fixed-step path (Th2: no
full voltage host pull when device detect succeeds). Host traffic each step is
flags + hit **slot indices** for deliver, only as needed.

---

## 2. Why no InputPreSyn in NEURON

CoreNEURON splits:

| CoreNEURON type | Role | On GPU threshold loop? |
|-----------------|------|-------------------------|
| `PreSyn` | Local output sources with voltage | Yes (`0 .. n_real_output`) |
| `InputPreSyn` | Remote/external input sources | No |

NEURON has a **single** `PreSyn` type. Remote/gid input is not a separate
class; those sources either lack `thvar_` or are not on `psl_thr_`.

**Th0 mapping (no new types):**

| Concept | NEURON representation |
|---------|------------------------|
| Detectable output source | `PreSyn` on this thread’s `psl_thr_` with modern SoA `thvar_` |
| Non-detectable / remote input | Not in the threshold slot table |

GPU detection never needs `InputPreSyn`. It only needs the compact **detect
set** below.

---

## 3. Th0 contract

### 3.1 Sole detect set = threshold slot table

Per `NrnThread`, the **only** set of sources subject to voltage threshold
detection on the gpu-native path is the rebuilt **slot table**:

```text
ThresholdPresynSlot[i] = {
  thvar_row,    // index into this thread’s node voltage SoA (0 .. nt->end)
  threshold,    // PreSyn::threshold_ copy at rebuild time
  flag,         // hysteresis for pscheck (0 = below, 1 = above)
  presyn,       // host-only PreSyn* for deliver (not used in detect kernel)
}
```

**Eligibility** (implemented by `collect_threshold_presyn_slots`):

- Listed on `NetCvodeThreadData::psl_thr_` for this thread
- `ps->nt_ == nt`
- `ps->thvar_` non-null
- `thvar_` refers to modern node voltage SoA
- `thvar_row = thvar_.current_row() - nt->_node_data_offset`

**Not** in the table: artcell-only / gid shells without `thvar_`, other threads’
PreSyns, remote input paths.

**Invariant:** On the gpu-native path, when the table path runs successfully
(`check_thresh_presyn_on_device` returns true), host code **must not**
re-walk `psl_thr_` for those SoA `thvar_` PreSyns (already skipped in
`NetCvode::check_thresh` via `gpu_thresh_handled`). The slot table is the sole
detect set.

### 3.2 Hit buffer = slot indices only

`NrnThread::_net_send_buffer` (int array) holds **indices into the slot table**,
not `PreSyn*` and not CoreNEURON’s absolute `presyns[]` array index.

```text
// after detect
for j in 0 .. _net_send_buffer_cnt-1:
  slot = _net_send_buffer[j]     // 0 .. n_slots-1
  deliver(slots[slot].presyn)
```

Semantics match CoreNEURON’s hit list (ints → host deliver), with NEURON’s
indirection through the compact table because PreSyn objects are not a
device-flat array.

**Naming (do not confuse):**

| Buffer | Owner | Purpose |
|--------|--------|---------|
| `NrnThread::_net_send_buffer` | Thread | **Threshold hit list** (this doc) |
| `Memb_list::_net_send_buffer` | Mechanism | MOD `net_send` / `net_event` from ACC kernels |

### 3.3 Deliver = host only

Spike side effects always run on the host:

- `PreSyn::send` → NetCon fanout, SelfEvents, MPI, etc.
- Flag write-back to `PreSyn::flag_` after detect (`sync_threshold_presyn_flags`)

Device (Th1+) only: `pscheck`, update `flag[]`, atomic fill of hit buffer.

### 3.4 Rebuild policy

Invalidate and rebuild slot tables when topology / threshold list / node sort
changes (`invalidate_threshold_tables`). After rebuild, re-copyin SoA columns
used by detect (`thvar_row`, `threshold`, `flag`).

---

## 4. CoreNEURON correspondence

| CoreNEURON | NEURON Th0 |
|------------|------------|
| `presyns[0:n_real_output)` | Slot table `0:n_slots` |
| `thvar_index_` | `thvar_row` |
| `presyns_helper[i].flag_` | Compact `h_flag[i]` (+ sync to `PreSyn::flag_`) |
| `nt->_net_send_buffer[j] = i` (PreSyn array index) | `nt->_net_send_buffer[j] = i` (**slot** index) |
| Host `presyns[i].send(...)` | Host `deliver_threshold_spike(..., slots[i].presyn, ...)` |
| `InputPreSyn` | Not modeled; excluded by eligibility |

---

## 5. Implementation stages

| Stage | Content | Status |
|-------|---------|--------|
| **Th0** | This contract; slot table = sole detect set; buffer = slot indices; host deliver | **Done** |
| **Th1** | OpenACC parallel detect over slots (device `vec_v`, atomic capture into hit buffer) | **Done** (`check_thresh.cpp`) |
| **Th2** | Skip voltage host sync when Th1 succeeds (lazy pull only for host PreSyn/WATCH) | **Done** |
| **Th3** | Ringtest 100 still green after Th0–Th2; record traffic notes | **Done** (2026-07-25) |
| **Th4** | Traub-scale threshold load | Pending |

### Th1 kernel (implemented)

```text
// device, after post_solve voltages are resident
for i in 0 .. n_slots-1:
  if pscheck(vec_v[thvar_row[i]], threshold[i], &flag[i]):
    atomic capture idx = cnt++
    nsbuffer[idx] = i
// host
update flag[] and nsbuffer
for each hit: deliver_threshold_spike(nt, slots[i].presyn, teps)
```

WATCH / non-voltage conditions stay host-only initially (same as CoreNEURON’s
pragmatic split).

---

## 6. Gate E (qualification)

`threshold_detection_on_device(nt)` is true when every threshold PreSyn on the
thread has a modern SoA voltage handle (so a slot table can be built with
integer rows). When `check_thresh_presyn_on_device` returns true under
gpu-native, Th1 runs the OpenACC detect kernel over that table.

---

## 7. Current code map

| API | Role under Th0/Th1 |
|-----|----------------|
| `collect_threshold_presyn_slots` | Build detect set from `psl_thr_` |
| `ThreadThresholdTable` in `check_thresh.cpp` | Per-thread slot + column storage |
| `NrnThread::_net_send_buffer` | Hit list of **slot** indices |
| `deliver_threshold_spike` | Host deliver one PreSyn |
| `sync_threshold_presyn_flags` | Hysteresis back to `PreSyn::flag_` |
| `check_thresh_presyn_on_device` | Th1: OpenACC detect; host flag/hit update + deliver |
| `NetCvode::check_thresh` | Th2: device detect first; lazy host `vec_v` pull |

---

## 8. Th2 host voltage pull policy

`NetCvode::check_thresh` runs device detect first. It calls
`sync_voltages_to_host_before_check_thresh` **only** if:

1. Device table path did not handle (`check_thresh_presyn_on_device` false) and
   host `PreSyn::check` will run, or
2. A non-skipped host PreSyn remains (non-modern `thvar_`), or
3. A WATCH condition will be evaluated on the host.

Ringtest (all modern SoA PreSyn, no WATCH) skips the full `vec_v` pull.

---

## 9. Th3 qualification (ringtest after Th0–Th2)

Harness (gpu-native, permute=2):

```bash
source ~/neuron/bin/nrnenv nrngpu build-gpu
export NRN_NATIVE_GPU_DEVICE_NONVINT=1 NRN_GPU_BACKEND_TEST=native NRN_GPU_PERMUTE=2
cd build-gpu/test/external_ringtest/neuron_gpu_native_mpi
./prcellstate_native_gpu.sh 32 <tstop> [0]
```

| tstop | Result (2026-07-25, Th2 stack) |
|-------|--------------------------------|
| **0.025** | GREEN — `dV=0`, matrix noise ~1e-15 |
| **1.0** | GREEN — `dV=0` |
| **1.025** | GREEN — `dV=0` (first NetCon) |
| **100** | GREEN — **688 spikes** both sides; threshold `dV=0`; max \|d\| ~1e-13 |

**Traffic notes (ringtest, not a microbench):**

- Detect set size ≪ CoreNEURON Traub: one threshold voltage per ring cell on the
  thread’s `psl_thr_` slice; hit rate is sparse relative to step count.
- Per step on the green path: OpenACC `pscheck` over slots + host update of
  flags/hits + host deliver only on crossings — **no** full `vec_v` host pull
  (Th2).
- GPU wall ~9–10 s for `tstop=100` on this machine is dominated by the fixed
  step (cur/jacob/solve), not threshold traffic. A dedicated threshold microbench
  is deferred to **Th4** (Traub-scale load) where slot count and crossing rate
  matter.

---

## 10. Non-goals (Th0–Th3)

- InputPreSyn type or dual PreSyn hierarchy
- Device-side NetCon fanout / MPI
- WATCH on device
- Traub-scale threshold microbench (Th4)
