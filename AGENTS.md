# Agent rules — NEURON + NMODL on CPU (heap-free base)

Read **`GROK-NMODL-CPU.md`** first on every new session.

| Item | Value |
|------|--------|
| Tree | `~/neuron/nrnnmodl` |
| Branch base | `local/cpu-net-soa-heap-free` (network SoA / weight ABI) |
| Focus | NMODL-generated mechanisms for **NEURON CPU** (Traub 82894 stress test) |
| Not this tree | Native GPU Traub / Th4 (`~/neuron/nrngpu`); heap-free SoA feature work itself |

Sibling handoff for the network SoA / heap-free **platform** (not NMODL parity):  
`GROK-NETWORK-SOA.md` and `doc/network-soa/` — read if ABI / Weight SoA / NET_RECEIVE issues appear.

## Do

- Fix NMODL → NEURON host codegen, install headers, `nrnivmodl`, registration.
- Compare to **NEURON + nocmodl on this same install/branch** (primary oracle).
- Keep NMODL weight/NET_RECEIVE codegen aligned with heap-free ABI (`weight_index`, `_nrn_netrec_wsoa`, FOR_NETCONS bases).
- Commit in this repo when the user asks for a milestone commit; no push unless asked.

## Do not

- Resume Traub native GPU / `NRN_GPU_ALLOW_UNQUALIFIED` / Th4 here.
- Treat CoreNEURON GPU as the first correctness bar for NEURON NMODL.
- Scope-creep into new heap-free network SoA features unless required for NMODL CPU parity.
- Use master/stock NEURON as the long-term integration base for this project.
