.. _native-gpu-fixed-step:

Native GPU fixed-step (Phase B)
###############################

This page is the **scope contract** for NEURON's native GPU backend
(``gpu.backend="native"``). It describes what Phase B supports, what is
explicitly excluded, how a fixed-step timestep is organized, and how to
configure runtime options.

For CTest commands, ringtest benchmarks, and CI troubleshooting, see
:doc:`gpu-testing`.

Overview
********

When NEURON is built with ``-DNRN_ENABLE_GPU=ON`` (NVHPC + OpenACC), models
can run fixed-step ``pc.psolve`` on the GPU **without enabling CoreNEURON at
runtime** (``coreneuron.enable=False``). That is different from the CMake
default ``NRN_ENABLE_CORENEURON=OFF`` — see :ref:`native-gpu-build-runtime`.

.. code-block:: python

   from neuron import h, gpu

   pc = h.ParallelContext()
   gpu.enable = True
   gpu.backend = "native"
   pc.psolve(100)

Phase B delivers CoreNEURON-parity integration for single-process modtests
(19 ``*_py_gpu_native`` ctests) while keeping cross-rank spike scheduling on
the CPU — the same split CoreNEURON uses.

Phase B contract
****************

**In scope**

- Fixed-step ``pc.psolve`` and ``h.fadvance`` on the native path
- OpenACC matrix solve (interleaved when permuted)
- Mechanism state on device (padded ``Memb_list`` upload)
- Post-solve on device: voltage update, fast_imem, capacity current
- ``NET_RECEIVE`` mechanism computation on GPU during the step
- CPU spike priority queues + MPI exchange at the minimum NetCon delay interval
- GPU ``net_send`` buffering; cross-cell events to CPU/MPI; self-events may flush on device
- Gap junctions: device voltage pull, sparse MPI transfer, host fallback where required
- Batch host download API for ``Vector.record`` and graphs (``download_flush_interval``)

**Out of scope (deferred)**

- CVode / IDA on native GPU (enabling native GPU with CVode active raises an error)
- GPU spike priority queue
- Full multi-thread OpenACC maturity (``pc.nthread(1)`` is the validated default)
- ``sparse13``, extracellular, and LFP post-solve hooks on device (host fallback)
- MPI-native modtest expansion beyond current parity set

Supported and unsupported matrix
********************************

+-------------------------------+------------------+---------------------------+
| Feature                       | Native GPU       | Notes                     |
+===============================+==================+===========================+
| Fixed-step ``pc.psolve``      | Supported        | Primary Phase B path      |
+-------------------------------+------------------+---------------------------+
| ``h.fadvance``                | Supported        | Same dispatch as psolve   |
+-------------------------------+------------------+---------------------------+
| CVode / IDA                   | **Unsupported**  | Use CPU or CoreNEURON     |
+-------------------------------+------------------+---------------------------+
| Mechanisms (NMODL)            | Supported        | OpenACC codegen path      |
+-------------------------------+------------------+---------------------------+
| ``NET_RECEIVE``               | Supported        | Computed on GPU           |
+-------------------------------+------------------+---------------------------+
| Cross-rank NetCon spikes      | CPU queues + MPI | Sparse at min delay       |
+-------------------------------+------------------+---------------------------+
| ``net_send`` self-events      | GPU buffer       | May flush on device       |
+-------------------------------+------------------+---------------------------+
| Gap junctions                 | Supported        | Sparse MPI voltage/ion    |
+-------------------------------+------------------+---------------------------+
| ``Vector.record``             | Supported        | Batch download API        |
+-------------------------------+------------------+---------------------------+
| HOC graphs                    | Supported        | Flush pulls GPU state     |
+-------------------------------+------------------+---------------------------+
| ``pc.nthread(n>1)``           | Experimental     | One-time warning emitted  |
+-------------------------------+------------------+---------------------------+
| ``use_sparse13``              | Host fallback    | Per-step RHS host pull    |
+-------------------------------+------------------+---------------------------+
| Extracellular / LFP hooks     | Host fallback    | Post-solve on host        |
+-------------------------------+------------------+---------------------------+
| CoreNEURON embedded backend   | Separate path    | ``gpu.backend=coreneuron``|
+-------------------------------+------------------+---------------------------+

Conceptual layers
*****************

Use these levels when reading code, tests, or the development journal (PR7):

+-------+--------------------+-----------------------------------------------+
| Level | Name               | Key artifacts                                 |
+=======+====================+===============================================+
| L0    | Build              | ``NRN_ENABLE_GPU``, ``NRN_ENABLE_CORENEURON``, NVHPC |
+-------+--------------------+-----------------------------------------------+
| L1    | Runtime API        | :mod:`neuron.gpu`, ``ParallelContext`` GPU    |
+-------+--------------------+-----------------------------------------------+
| L2    | Device lifecycle   | ``device_state``, ``upload`` / teardown       |
+-------+--------------------+-----------------------------------------------+
| L3    | Timestep dispatch  | ``fadvance.cpp`` → ``fadvance_gpu.cpp``       |
+-------+--------------------+-----------------------------------------------+
| L4    | Integration step   | Matrix setup, solve, post-solve               |
+-------+--------------------+-----------------------------------------------+
| L5    | Host fallbacks     | ``post_solve_needs_host_fallback()``         |
+-------+--------------------+-----------------------------------------------+
| L6    | Communication      | Spike queues (CPU), gap MPI, ``NetSendBuffer``|
+-------+--------------------+-----------------------------------------------+
| L7    | Recording          | ``download.cpp``, batch flush intervals       |
+-------+--------------------+-----------------------------------------------+
| L8    | Verification       | ``*_py_gpu_native`` modtests, unit tests      |
+-------+--------------------+-----------------------------------------------+

One fixed step on the native path
*********************************

At a high level, thread 0 (and peers) execute:

.. mermaid::

   flowchart TD
     A[deliver_net_events: CPU queues → targets] --> B[setup_tree_matrix + mechanism currents]
     B --> C[GPU matrix solve]
     C --> D{post_solve host fallback?}
     D -->|no| E[post_solve_on_device: V, fast_imem]
     D -->|yes| F[host nrn_update_voltage path]
     E --> G{gap junctions?}
     F --> G
     G -->|yes| H[sync voltages + MPI transfer]
     G -->|no| I[lastpart: events, vecplay]
     H --> I
     I --> J{download flush interval?}
     J -->|yes| K[batch_download_to_host]
     J -->|no| L[defer to next flush / psolve end]
     K --> M[advance step counter]
     L --> M

Spike exchange with MPI occurs at the **minimum NetCon delay** cadence (often
many fixed steps), not every ``dt``.

Runtime configuration
*********************

Primary API: :mod:`neuron.gpu` (HOC equivalents on ``ParallelContext``).

.. code-block:: python

   from neuron import gpu

   gpu.enable = True
   gpu.backend = "native"       # fixed-step native path
   gpu.permute = 2              # default when enable=True
   gpu.device_count = 0         # 0 = all GPUs on the node
   gpu.download_flush_interval = 1  # steps between host pulls (0 = psolve end only)

Context manager (mirrors ``coreneuron()``):

.. code-block:: python

   with gpu(enable=True, backend="native", download_flush_interval=10):
       pc.psolve(tstop)

Build-time vs runtime configuration
***********************************

.. _native-gpu-build-runtime:

The old single truth table mixed **CMake build options** (fixed at configure
time) with **Python/HOC runtime switches** (chosen per simulation). They are
independent layers.

Build-time (CMake)
------------------

These options decide which code is compiled into your ``special`` binary. They
are **not** implied by ``gpu.backend`` or ``coreneuron.enable``.

+-------------------------------+--------------------------------+--------------------------------+
| ``NRN_ENABLE_GPU``            | ``NRN_ENABLE_CORENEURON``      | Configure result               |
+===============================+================================+================================+
| OFF (default)                 | OFF (default)                  | CPU NEURON only; ``gpu.*``     |
|                               |                                | has no effect at runtime       |
+-------------------------------+--------------------------------+--------------------------------+
| OFF                           | ON                             | CoreNEURON embedded path only  |
|                               |                                | (no native GPU dispatch)       |
+-------------------------------+--------------------------------+--------------------------------+
| ON                            | OFF                            | **Configure fails** (Phase A–B |
|                               |                                | transition; CMake still        |
|                               |                                | requires CoreNEURON)           |
+-------------------------------+--------------------------------+--------------------------------+
| ON                            | ON                             | Both paths available at        |
|                               |                                | runtime (typical dev/CI build) |
+-------------------------------+--------------------------------+--------------------------------+

Most users leave ``NRN_ENABLE_CORENEURON`` at its default **OFF**. Today you
cannot combine that default with ``NRN_ENABLE_GPU=ON``: you must pass
``-DNRN_ENABLE_CORENEURON=ON`` to configure a native-GPU-capable build. That
builds the CoreNEURON library into the install tree, but you still choose at
**runtime** whether ``pc.psolve`` embeds CoreNEURON or runs the native path.

Runtime dispatch (``pc.psolve``)
--------------------------------

``gpu.backend`` and ``coreneuron.enable`` answer different questions:

- ``coreneuron.enable`` — should ``pc.psolve`` **transfer the model and run
  inside CoreNEURON**?
- ``gpu.enable`` + ``gpu.backend`` — on the **classic NEURON** path, should
  fixed-step integration run on the **native GPU** backend?

``pc.psolve`` checks ``coreneuron.enable`` **first**:

1. ``coreneuron.enable=True`` → CoreNEURON embedded (``nrncore_psolve``).
   GPU execution inside CoreNEURON is controlled by ``coreneuron.gpu`` (legacy;
   sets ``gpu.enable`` and ``gpu.backend="coreneuron"``), not by
   ``gpu.backend="native"``.
2. ``coreneuron.enable=False`` → classic NEURON (``netpar_solve``). Native
   GPU runs only when ``gpu.enable=True`` **and** ``gpu.backend="native"``.

Requires ``NRN_ENABLE_GPU=ON`` at build time; otherwise ``gpu.enable`` is a
no-op and every row below is CPU NEURON.

+---------------------+--------------+----------------+---------------------------+
| coreneuron.enable   | gpu.enable   | gpu.backend    | Effective ``pc.psolve``   |
+=====================+==============+================+===========================+
| False               | False        | *              | CPU NEURON                |
+---------------------+--------------+----------------+---------------------------+
| False               | True         | ``native``     | **Native GPU** fixed-step |
+---------------------+--------------+----------------+---------------------------+
| False               | True         | ``coreneuron`` | CPU NEURON (backend has   |
|                     |              |                | no effect without embed)  |
+---------------------+--------------+----------------+---------------------------+
| True                | False        | *              | CoreNEURON embedded, CPU  |
+---------------------+--------------+----------------+---------------------------+
| True                | True         | ``coreneuron`` | CoreNEURON embedded, GPU  |
|                     |              |                | (``coreneuron.gpu`` /     |
|                     |              |                | ``--gpu`` launcher arg)   |
+---------------------+--------------+----------------+---------------------------+
| True                | True         | ``native``     | CoreNEURON embedded       |
|                     |              |                | (**not** native GPU;       |
|                     |              |                | ``coreneuron.enable``     |
|                     |              |                | wins over ``gpu.backend``)|
+---------------------+--------------+----------------+---------------------------+

**Phase B native recipe** (after a GPU-capable build):

.. code-block:: python

   from neuron import h, gpu

   # coreneuron.enable stays False (default)
   gpu.enable = True
   gpu.backend = "native"
   pc.psolve(tstop)

Download flush interval
***********************

Recording and HOC reads of GPU-resident state use a **batch download** API.
By default (``download_flush_interval=1``), post-solve voltages and fast_imem
are pulled to the host every step. Set ``0`` to defer all pulls until
``psolve`` ends (best throughput; recordings see data only at flush boundaries).

HOC: ``pc.gpu_download_flush_interval(interval)``

Threading
*********

Native GPU modtests run with ``OMP_NUM_THREADS=1`` and ``pc.nthread(1)``.
Using ``pc.nthread(n)`` for ``n > 1`` emits a one-time warning: per-thread
OpenACC CUDA contexts are not fully validated on all platforms.

Testing
*******

Quick parity check after building with GPU tests enabled:

.. code-block:: console

   ctest -N -R '_py_gpu_native'
   ctest --output-on-failure -R '_py_gpu_native'

Broader GPU regression:

.. code-block:: console

   ctest --output-on-failure -R gpu

See :doc:`gpu-testing` for ringtest, MPI device assignment, and NVHPC linker
notes.

Related documentation
*********************

- :doc:`/dev/native-gpu-adoption/00-overview` — Phase B development journal (design, commits)
- :ref:`cmake-nrn-enable-gpu-option` — CMake configure
- :doc:`/nmodl/gpu_codegen` — mechanism OpenACC codegen
- :doc:`gpu-testing` — CTest and benchmark workflows