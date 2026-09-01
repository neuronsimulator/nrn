# L2 — Dual-write and the weight heap

Brevity: medium. Topology: [topology.md](topology.md).

---

## 1. Dual-write rule of thumb

| Plane | Primary for… | Examples |
|-------|----------------|----------|
| **Weight SoA** | HOC `weight[]`, SaveState values, future GPU upload | `weight_soa_`, `Model::weights()` |
| **`weight_` heap** | Generated MOD `double*` ABI, FOR_NETCONS walks, `net_send` capturing `_w` | `NetCon::weight_` |
| **Pointers (`target_`, `src_`, `dil_`)** | Queue / HOC / disconnect until dual-read done | Legacy shells |
| **Indices (`Target`, `WeightIndex`, `NcIndex`)** | CoreNEURON-shaped hot path once dual-read | SoA columns |

**Invariant:** after any path that mutates weights for HOC visibility, SoA and heap agree *or* the next deliver materializes SoA→heap before MOD runs and heap→SoA after.

---

## 2. Why “short-lived materialize only” failed

Passing a **temporary** buffer into `pnt_receive` broke SelfEvent identity: nocmodl does `net_send(..., _w, ...)` and SaveState keyed SelfEvents by that pointer (`weight2netcon`).

**Current policy:** long-lived `weight_` is **MOD scratch** (stable identity for `net_send`). SoA remains HOC-primary; deliver does SoA→heap→MOD→heap→SoA (or equivalent owner path in `nrn_pnt_receive_by_weight_index`).

---

## 3. SaveState / BBSaveState (done on this branch)

- **Values:** save prefer SoA; restore write heap **and** SoA.  
- **SelfEvent identity:** NetCon object index / DEList `ncindex` + `weight_index`, not sole reliance on heap pointer (pointer still works as fallback while heap lives).

---

## 4. Roadmap to full heap free (if chosen)

Rough commit series (each gateable):

1. **SelfEvent never stores identity-only-as-temp-pointer** — always set `weight_index_` (done in dual-write spirit); never pass non-owner buffers into `pnt_receive` if MOD can net_send.  
2. **FOR_NETCONS** — either CoreNEURON-style weight index perm, or generate index-based FOR_NETCONS; stop requiring foreign NetCon `weight_` bases.  
3. **INITIAL** — `pnt_receive_init` by index or materialize into owner heap only.  
4. **Stop allocating `new double[cnt_]`** — optional thread-local scratch of `max(pnt_receive_size)` for MOD only if ABI still needs `double*`.  
5. **Delete `weight_` field** — after (1–4) and tests (stdp/FOR_NETCONS, SaveState ring, netrec init).

Until (2)–(3), “full drop” is premature.

---

## 5. PR strategy (honest)

| Option | Pros | Cons |
|--------|------|------|
| **PR series without heap free** | Reviewable design; CI; early feedback; doesn’t pretend GPU is done | Master doesn’t get space win yet |
| **Hold until heap free** | Stronger “finished dual-write” story | Large blocked-on-codegen risk; long private branch |
| **Hold until GPU** | End-to-end performance story | Couples orthogonal tracks; slowest |

Recommendation: **document dual-write+sort as a reviewable PR (or draft PR), keep landing bar explicit** (“not for merge until X”). Heap-free and GPU as **follow-on PR chains**, not prerequisites for *having* a PR.

If the only merge criterion is measured space/time, then next engineering is **benchmarks** (ringtest CPU memory + spike path) *and* heap-free or GPU — not more silent dual-write surface area.
