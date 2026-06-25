.. _native-gpu-phase-c:

Native GPU Phase C — CoreNEURON runtime parity
###############################################

This page is the **scope contract** for Phase C of NEURON's native GPU backend
(``gpu.backend="native"``). Phase C builds on :doc:`native-gpu-fixed-step`
(Phase B) and targets **runtime feature parity with CoreNEURON GPU** while
keeping ``coreneuron.enable=False``.

Branch: ``hines-grok/feature/neuron-native-gpu-phase-c`` (based on Phase B tip).

Development journal: :doc:`/dev/native-gpu-adoption/PHASE-C-OVERVIEW`.

Overview
********

Phase B certified single-process, fixed-step ODE tree models on the native GPU
path (19 ``*_py_gpu_native`` ctests). Gap models with ``pc.setup_transfer()``
run a **full CPU fixed-step fallback** for correctness.

Phase C closes the gap to **embedded CoreNEURON GPU** for the features
CoreNEURON actually supports:

- GPU fixed-step integration with **hybrid partrans** (device gather/scatter,
  host ``MPI_Alltoallv`` when ``nrnmpi_numprocs > 1``)
- **MPI** multi-rank validation (native modtests + ringtest ``-gap``)
- **Multi-thread** OpenACC hardening (``pc.nthread(n>1)``)
- **LFP / fast_imem** path aligned with CoreNEURON (GPU membrane currents,
  CPU LFP calculation)

Phase C does **not** aim to exceed CoreNEURON capability (no CVode, extracellular,
``LinearMechanism``, RxD, or GPU spike priority queues on either stack).

Relationship to CoreNEURON GPU
******************************

CoreNEURON GPU is the reference architecture for Phase C. The table below
states where work executes today on each stack and the Phase C target for native
GPU.

+---------------------------+------------------+------------------+------------------+
| Feature                   | CoreNEURON GPU   | Native Phase B   | Native Phase C   |
+===========================+==================+==================+==================+
| Fixed-step integration    | GPU              | GPU (no gaps)    | GPU (incl. gaps) |
+---------------------------+------------------+------------------+------------------+
| Cross-rank NetCon spikes  | CPU + MPI        | CPU + MPI        | CPU + MPI        |
+---------------------------+------------------+------------------+------------------+
| Gap gather (sources)      | GPU → host buf   | CPU (fallback)   | GPU → host buf   |
+---------------------------+------------------+------------------+------------------+
| Gap MPI exchange          | Host MPI         | Host MPI         | Host MPI         |
+---------------------------+------------------+------------------+------------------+
| Gap scatter (targets)     | GPU              | CPU (fallback)   | GPU              |
+---------------------------+------------------+------------------+------------------+
| ``pc.nthread(n>1)``       | Supported        | Experimental     | Validated        |
+---------------------------+------------------+------------------+------------------+
| MPI modtest parity        | ``*_py_gpu``     | 19 single-proc   | + MPI natives    |
+---------------------------+------------------+------------------+------------------+
| LFP from membrane current | CPU              | Not wired        | CPU (from GPU    |
|                           |                  |                  | ``fast_imem``)   |
+---------------------------+------------------+------------------+------------------+
| Extracellular / linmod    | Unsupported      | Unsupported      | Unsupported      |
+---------------------------+------------------+------------------+------------------+
| CVode / RxD               | Unsupported      | Unsupported      | Unsupported      |
+---------------------------+------------------+------------------+------------------+

Reference implementation for device partrans:
``src/coreneuron/network/partrans.cpp``, ``partrans_setup.cpp``.

Phase C work packages
*******************

Work is ordered by dependency. Each package is intended as a reviewable PR on
the Phase C branch.

C0 — Branch scaffolding
    Scope contract (this document), journal entry, Sphinx toctree. No runtime
    code.

C1 — Device partrans + restore GPU integration (**critical path**)
    Build setup-time index maps from NEURON ``data_handle`` sources/targets to
    device array offsets. Port CoreNEURON gather/scatter pattern. Remove the
    ``!nrnthread_v_transfer_`` CPU fallback gate in ``fadvance.cpp``.

C2 — MPI validation
    Native GPU variants of CoreNEURON MPI modtests (``spikes_mpi``,
    ``spikes_mpi_file_mode``, ``test_subworlds``, …). Ringtest
    ``neuron_gpu_native_mpi`` with ``-gap``.

C3 — Multi-thread OpenACC
    Harden per-thread CUDA streams around partrans; validate ``pc.nthread(n>1)``;
    reconcile modtests that use ``nthread>1`` (e.g. ``test_natrans``).

C4 — LFP / ``fast_imem``
    Ensure GPU ``fast_imem`` / ``i_membrane_`` recording on the native path.
    CPU LFP from downloaded currents (CoreNEURON-equivalent split). Restrict
    ``nrnthread_vi_compute_`` host post-solve to extracellular ``v+vext`` sources.

C5 — Performance sign-off
    Traub ``use_gap=1`` benchmark (raster + runtime vs Phase B CPU fallback).
    Reduce per-step matrix host sync where safe (``treeset.cpp``). Update Phase C
    checklist in the adoption journal.

CoreNEURON parity checklist
***************************

Phase C is **complete** when the items below are satisfied (unless explicitly
deferred in a follow-up issue).

+-------------------------------+-----------------------------------------------+
| Item                          | Verification                                  |
+===============================+===============================================+
| ODE tree, no gaps             | 19/19 ``*_py_gpu_native`` (no regression)     |
+-------------------------------+-----------------------------------------------+
| Gap junctions, single rank    | ``test_par_gj_native_gpu.py``; ringtest       |
|                               | ``-gap -gpu-native`` raster match             |
+-------------------------------+-----------------------------------------------+
| Gap junctions, Traub 1/10     | 7873 spikes vs NEURON CPU                     |
+-------------------------------+-----------------------------------------------+
| MPI spikes                    | ``spikes_mpi*_py_gpu_native``                 |
+-------------------------------+-----------------------------------------------+
| MPI subworlds / inputpresyn   | Matching CoreNEURON MPI native tests          |
+-------------------------------+-----------------------------------------------+
| MPI + gaps                    | ``neuron_gpu_native_mpi`` with ``-gap``       |
+-------------------------------+-----------------------------------------------+
| Multi-thread                  | ``test_natrans_py_gpu_native`` at ``nthread>1``|
|                               | without fallback to CPU-only path             |
+-------------------------------+-----------------------------------------------+
| ``fast_imem`` / recording     | ``fast_imem_py_gpu_native`` + LFP smoke test  |
+-------------------------------+-----------------------------------------------+

Design decisions (Phase C defaults)
***********************************

These defaults apply unless review changes them before C1 implementation.

**Parity definition**
    Match **CoreNEURON GPU runtime features**, not “everything on GPU.” MPI spike
    exchange and LFP dot products remain on CPU, as in CoreNEURON.

**Partrans refactor**
    Add a native GPU partrans layer with **index-based gather/scatter** (CoreNEURON
    pattern) alongside existing NEURON ``partrans.cpp`` setup. Do not merge
    CoreNEURON and NEURON partrans into one binary in Phase C.

**Thread policy**
    Target **validated** ``pc.nthread(n>1)`` on NVHPC CI; keep
    ``OMP_NUM_THREADS`` documented. Remove or downgrade the Phase B one-time
    warning once C3 tests pass.

**Extracellular gap sources**
    ``nrnthread_vi_compute_`` (``v+vext``) keeps host staging; plain voltage
    gaps use device post-solve after C1.

Explicit non-goals
******************

Deferred past Phase C (same as CoreNEURON exclusions unless noted):

- CVode / IDA on native GPU
- Extracellular mechanism, ``LinearMechanism``, GPU ``sparse13``
- RxD
- GPU spike priority queue
- GPU LFP kernels (CoreNEURON uses CPU LFP)
- ``-DNRN_ENABLE_GPU=ON`` without ``-DNRN_ENABLE_CORENEURON=ON`` at configure
- Interpreter callbacks during integration steps

Related documentation
*********************

- :doc:`native-gpu-fixed-step` — Phase B scope contract
- :doc:`/dev/native-gpu-adoption/00-overview` — Phase B development journal
- :doc:`/dev/native-gpu-adoption/PHASE-C-OVERVIEW` — Phase C journal entry
- :doc:`gpu-testing` — CTest and ringtest workflows
- :doc:`/coreneuron/compatibility` — CoreNEURON feature boundaries
