# GPU-native PreSyn threshold detection

**Status:** Th0 (contract) + **Th1** (OpenACC detect over slot columns + device
`vec_v`). Spike deliver remains host-only. **Th2** (skip voltage host pull when
Th1 succeeds) is not done.

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
| **Th2** | Skip voltage host sync when Th1 succeeds (threshold no longer forces full `vec_v` pull) | Pending |
| **Th3** | Ringtest 100 still green; optional traffic/microbench | Pending |
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

---

## 8. Non-goals (Th0/Th1)

- Removing voltage pull (Th2)
- InputPreSyn type or dual PreSyn hierarchy
- Device-side NetCon fanout / MPI
- WATCH on device
