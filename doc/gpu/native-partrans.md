# Native GPU parallel transfer (gap / source→target)

**Portfolio:** GPU-native (feature)  
**Tree:** `~/neuron/nrngpu`  
**Status:** S0–S2 green; **S3 partial (2026-07-30)** — multi-thread buffer path
(live re-bucket gather + per-thread target-mech sync) + NMODL ACC dptr
`storage_offset` fix; ringtest gap `nthread>1` product still residual (see below).

**Product policy (2026-07-30):** device gather/scatter under native is mandatory.
Silent no-op / host V pull when residency fails is **not** default. Opt-in debug:
`NRN_GPU_GAP_HOST_FALLBACK=1`. Same spirit as threshold detect fail-loud.  

**Related:** `doc/gpu/native-coreneuron-parity.md` (P2), CoreNEURON `src/coreneuron/network/partrans.cpp`

---

## Problem

`ParallelContext.source_var` / `target_var` + `setup_transfer` move **source RANGE values** into **target RANGE slots** each fixed step (gap junctions, natrans-style ion copies, …). Sources and targets may live on:

| Topology | Example |
|----------|---------|
| Same thread, same rank | Typical 1-thread ringtest `-gap` |
| Different threads, same rank | `pc.nthread(n>1)` |
| Different MPI ranks | Multi-rank gap / partrans |

Most tests exercise **single thread + single rank**. A design that special-cases that path as a pure on-device shortcut leaves the general buffer/MPI path poorly tested. This document chooses **one path first**.

---

## Decision: CoreNEURON-style buffering first

### Product path (implement first)

**Mimic CoreNEURON:** every source→target edge goes through **sparse gather → host staging buffers → (MPI if needed) → device scatter**, including when source and target are on the **same thread**.

Reasons:

1. **One code path** for 1-rank/1-thread, multi-thread, and multi-rank — the cases most tests hit still exercise pack/unpack and buffer indices.
2. **Fan-out is natural.** One interior source often feeds several targets (e.g. four neighbors in a rectangular array). CoreNEURON already stages unique sources then fans out into `outsrc` / target indices.
3. **MPI stays on the CPU** (gather results and `insrc` are host-visible for `MPI_Alltoallv`).
4. **Correctness before micro-optimization.** Sparse host hops for gather/insrc are the CoreNEURON guide; they are far cheaper than full SoA mirrors.

### Deferred performance enhancement (explicit non-goal for first green)

**Same-thread pure on-device copy** (no host hop for edges with `src_tid == tar_tid`) is a **later** measured optimization. It must not be the first product path, and when added it should be optional / gated so the buffer path remains the default in tests unless explicitly enabled.

---

## What CoreNEURON actually does (reference)

Code: `src/coreneuron/network/partrans.cpp`, setup in `partrans_setup.cpp`, step split in `fadvance_core.cpp`.

### Step timing

```text
nrn_fixed_step_thread(...)      // through update V; lastpart deferred if gaps
if (nrn_have_gaps):
    nrnmpi_v_transfer()         // global controller
    nrn_fixed_step_lastpart()   // nonvint starts with nrnthread_v_transfer
```

### Per-step buffer path (GPU and CPU; GPU uses `if (compute_gpu)`)

```text
1. Per thread (device if compute_gpu):
     src_gather[i] = _data[src_indices[i]]     // any RANGE in thread _data
2. Device → host: src_gather only (sparse)
3. Host: pack outsrc_buf from gathers (fan-out OK)
4. Host: MPI_Alltoallv, or 1-rank copy insrc ← outsrc
5. Host → device: insrc_buf
6. Per thread (device):
     _data[tar_indices[j]] = insrc_buf[insrc_indices[j]]
```

Important facts for native designers:

| Fact | Implication |
|------|-------------|
| Same-thread edges still use buffers | Do not require a pure-GPU shortcut for parity with CoreNEURON |
| Inter-thread couples via host `outsrc`/`insrc` | No peer GPU transfer required for first product |
| MPI is host-only | Same for native |
| Sources are indices into `_data`, not “voltage only” | Non-V RANGE (ions, …) use the same tables via type/index setup |
| Scatter writes device memory CURRENT already reads | No full mechanism SoA upload after transfer |

---

## NEURON CPU today (baseline)

`src/nrniv/partrans.cpp` + `nrn_fixed_step` in `fadvance.cpp`.

- Host `data_handle` sources/targets; `thread_transfer`: `*(tv)=*(sv)`.
- `nrnmpi_v_transfer_` only when `nhost > 1`; 1-rank is direct host pointer copy.
- Non-V sources: `non_vsrc_update_info_` (mech type + field index).
- Extracellular `v+vext`: `source_vi_buf_` + `nrnthread_vi_compute_`.

CPU is the **raster correctness** oracle. CoreNEURON GPU is the **traffic / residency** guide.

---

## Native NEURON design (first product)

### Goals

1. Source RANGE → target RANGE for **any** RANGE (node V default; also ionic concentrations, etc.).
2. Correct for same-thread, cross-thread, and MPI (all via the buffer path initially).
3. MPI only on CPU.
4. **No full mechanism SoA** host↔device for transfer (that clobbers device-authoritative STATE).
5. Sparse traffic only: source gather + `insrc` (and index tables).

### Source / target locations (setup)

After structure is stable (`mk_ttd` / post-partition; `Node::_nt` and permute valid):

```text
SourceLoc / TargetLoc:
  NodeVoltage { tid, v_node_index }
  MechRange   { tid, mech_type, field_index, array_index, instance_index }
              // resolved to device-present SoA column + row
```

- Build from existing `source_var` / `target_var` bookkeeping and `non_vsrc_update_info_`.
- One **sid** has one source owner; many targets may share a sid (fan-out).
- Rebuild when structure / permute / `nthread` changes.

### Rank-local runtime tables

```text
// Host (MPI + pack)
outsrc_buf_[n_out]
insrc_buf_[n_in]
outsrccnt_ / outsrcdspl_ / insrccnt_ / insrcdspl_   // existing MPI layout OK

// Per thread (device kernels + sparse H↔D)
src_indices / SourceLoc list     // what to gather from this thread
src_gather[tid][i]               // device buffer of gathered doubles
gather2outsrc / outsrc_indices   // host pack map (fan-out)

tar SourceLoc/TargetLoc list
insrc_indices[j]                 // which insrc slot → target j
```

NEURON does not have CoreNEURON’s single `_data` blob; gather/scatter kernels branch on `NodeVoltage` vs `MechRange` (and later ecell `v+vext` if product requires).

### Per-step algorithm (all edges, including same-thread)

Align with existing NEURON split:

```text
// After device post_solve (integration state device-authoritative)
// Phase G — device gather (every thread that owns sources)
for tid:
  device: src_gather[tid][i] = load(SourceLoc[i])

// Phase H — host pack + MPI (single controller, like today nrnmpi_v_transfer_)
wait streams
update host(src_gather[*])          // sparse
host pack outsrc_buf                // fan-out
if nhost > 1: MPI_Alltoallv
else:         insrc ← outsrc (or equivalent local pack so multi-thread still uses insrc)

// Phase I — device scatter (every thread that owns targets)
update device(insrc_buf)            // sparse prefix / full buf as sized
for tid:
  device: store(TargetLoc[j], insrc[insrc_indices[j]])
```

Wire as today:

```text
nrnmpi_v_transfer_   → Phase G (orchestrate) + H
nrnthread_v_transfer_ → Phase I on that thread (from nonvint / lastpart)
```

### Host-only source fallback (narrow)

If a source RANGE is **not** device-resident (host-only mech on native), gather that sid from host into `outsrc` without device gather. Prefer qualification / ACC for product mechs; do not use full SoA pull as the default.

### Extracellular / `v+vext`

Keep existing host `vi_compute` path or express as a special `SourceLoc` that reads the same host buffer CoreNEURON would stage; do not block V-only gaps on ecell.

---

## Anti-patterns

| Anti-pattern | Why |
|--------------|-----|
| Host `*(tv)=*(sv)` as primary native path | Host V / RANGE stale under device-authoritative state |
| Full mech SoA `update device` after scatter | Clobbers device STATE (observed ringtest death mode) |
| Full `vec_v` host pull every step for gaps | Correctness hammer; not sparse CoreNEURON shape |
| Pure same-thread GPU path as **first** product | Leaves buffer/MPI path under-tested while most tests are 1-thread |
| Bit-identity with CoreNEURON internals | Guide only; better sparse algorithms allowed if measured |

---

## Future enhancement: same-thread on-device (not first)

When product is green and measured:

```text
Optional: edges with src_tid == tar_tid && same rank
  → device kernel tar = load(src) without host hop
```

Constraints if added:

- Default **off** in ctest, or always dual-run buffer path in a debug build.
- Fan-out still correct (one source, N local targets).
- Must not diverge semantics from the buffer path.
- Document as Phase 4 / perf debt, not P2 correctness.

---

## Implementation slices

| ID | Work | Acceptance |
|----|------|------------|
| **S0** | Setup: `SourceLoc`/`TargetLoc` tables; sid fan-out; rebuild on structure change | Dump / asserts; no runtime change required |
| **S1** | Phase G+H+I for **NodeVoltage → target**, 1-rank (still full buffer path) | ACC HalfGap + ringtest `-gap` 1-rank spike match vs CPU / `spk2.gap.100ms.std.ref` |
| **S2** | Multi-rank MPI via existing host Alltoallv | **green** — `neuron_gpu_native_mpi_gap` 2-rank + sorted `spk2.gap.100ms.std.sorted.ref` (launch via `h.nrnmpi_init`, not `special -mpi`) |
| **S3** | Multi-thread via same buffers | **partial** — buffer path multi-thread gather/scatter fixed; ringtest `-gap -nt 2` still **64 vs 128** spikes (thread-1 cells silent). Residual: ELECTRODE HalfGap ACC multi-thread (not mailbox alone; g=0 still half). |
| **S4** | `MechRange` sources (natrans ions) | `test_natrans` native beyond nthread=1 |
| **S5** | Traffic audit; no full-V default; optional same-thread GPU shortcut | Measured; tests still cover buffer path |

---

## Prerequisites / related bugs

- `setup_transfer` must not assume `Node::_nt` is set before partition; GPU gather index tables must refresh in `mk_ttd` after threads exist.
- Ringtest gap native needs **NMODL OpenACC HalfGap** (or equivalent) for Gate B/C (`nrnivmodl -nmodl … --c acc --oacc`).
- Multi-rank on one GPU: `gpu_device_assign` / shared device; separate P2 row if residual.

---

## References

| Item | Path |
|------|------|
| CoreNEURON transfer | `src/coreneuron/network/partrans.cpp` |
| CoreNEURON setup | `src/coreneuron/network/partrans_setup.cpp` |
| CoreNEURON step | `src/coreneuron/sim/fadvance_core.cpp` (`nrn_fixed_step_minimal`) |
| NEURON transfer | `src/nrniv/partrans.cpp` |
| NEURON step | `src/nrnoc/fadvance.cpp` (`nrn_fixed_step`) |
| Native step | `src/neuron/gpu/fadvance_gpu.cpp` |
| Parity backlog | `doc/gpu/native-coreneuron-parity.md` Phase 2 |

---

## S3 residual (multi-thread product)

What landed for multi-thread **buffers**:

- Live re-bucket of voltage sources by `Node::_nt` each gather step (not rebuild-time only).
- Per-thread bulk `sync_gap_target_mechs_to_device` (was tid==0 only → other threads’ `vgap` never pushed under serial multi-thread).
- NMODL ACC: GPU `pdata` dptr full-table index uses `id + storage_offset` (was local `id` only → wrong area for electrode mechs on tid>0).

What still fails product:

- `ringtest -gpu-native -gap -nt 2`: **64 spikes vs CPU 128**; firing gids are only the first half (thread 0). Even with `g=0` (no coupling), GPU multi-thread fires 8 vs CPU 16 ring-leader spikes — so this is **not** only stale `vgap` transfer. Suspect remaining ACC multi-thread ELECTRODE/`present` issues for HalfGap CURRENT/JACOB on tid>1.
- No-gap multi-thread native remains green (688 spikes) — gap/HalfGap path is the differentiator.

## One-line Next

**S3 residual / S4:** fix ELECTRODE HalfGap ACC multi-thread (product `-gap -nt>1`); then S4 MechRange (natrans nthread>1).
