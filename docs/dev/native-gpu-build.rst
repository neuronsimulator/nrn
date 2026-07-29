.. _native-gpu-build:

Native GPU development build
############################

Practical recipe for configuring, installing, and running **NEURON native GPU**
(``gpu.backend="native"``) on a workstation with NVHPC and an NVIDIA GPU.

For the Phase B **scope contract** (supported features, timestep layout, runtime
API), see :doc:`native-gpu-fixed-step`. For CTest filters and ringtest flags, see
:doc:`gpu-testing`. For CMake option reference, see
:ref:`cmake-nrn-enable-gpu-option`. Qualification gates (A–E) are described in
``doc/gpu-step-qualification.md`` in the source tree.

Typical local layout
********************

Developers often keep a dedicated build directory next to the source tree:

.. code-block:: text

   ~/neuron/nrngpu/                 # worktree (example name)
   ├── build-gpu/                   # out-of-source build (often untracked)
   │   ├── grok-bld                 # optional local helper script (not in git)
   │   └── install/                 # CMAKE_INSTALL_PREFIX
   ├── docs/dev/native-gpu-build.rst
   └── GROK-GPU-NATIVE.md           # session handoff (integration branch)

The CMake flags below match a full MPI + tests development install (ringtest
and GPU ctests). A minimal recipe (MPI/tests/interviews/rx3d off) may appear in
local notes such as ``~/neuron/notes/gpu_workstation.md``; prefer this page for
native-GPU development that needs the external ringtest harness.

Configure and install
*********************

Requirements:

- NVIDIA GPU (example below uses compute capability **75**, e.g. T1000)
- **NVHPC** compilers (``nvc``, ``nvc++``, ``nvcc``) on ``PATH``
- **Ninja**, **CMake**, MPI (e.g. OpenMPI), Python 3

.. code-block:: bash

   # Example paths — adjust NVHPC and MPI roots for your machine
   export PATH=/opt/nvidia/hpc_sdk/Linux_x86_64/25.9/compilers/bin:$PATH
   export PATH=$HOME/soft/openmpi/bin:$PATH

   cmake -S . -B build-gpu -G Ninja \
     -DCMAKE_INSTALL_PREFIX=$PWD/build-gpu/install \
     -DCMAKE_BUILD_TYPE=RelWithDebInfo \
     -DCMAKE_C_COMPILER=nvc \
     -DCMAKE_CXX_COMPILER=nvc++ \
     -DCMAKE_CUDA_COMPILER=nvcc \
     -DCMAKE_CUDA_ARCHITECTURES=75 \
     -DNRN_ENABLE_CORENEURON=ON \
     -DNRN_ENABLE_GPU=ON \
     -DNRN_GPU_BACKEND=OpenACC \
     -DCORENRN_ENABLE_GPU=ON \
     -DNRN_ENABLE_MPI=ON \
     -DNRN_ENABLE_TESTS=ON \
     -DNRN_ENABLE_INTERVIEWS=ON \
     -DNRN_ENABLE_RX3D=ON \
     -DNRN_ENABLE_MOD_COMPATIBILITY=OFF \
     -DPYTHON_EXECUTABLE="$(which python3)"

   cmake --build build-gpu --parallel "$(nproc)" --target install

Notes:

- ``NRN_ENABLE_GPU=ON`` currently requires ``NRN_ENABLE_CORENEURON=ON`` at
  configure time (Phase A–B transition). That builds CoreNEURON into the
  install; runtime still chooses **native** vs CoreNEURON via
  ``gpu.backend`` / ``coreneuron.enable`` — see :ref:`native-gpu-build-runtime`.
- ``NRN_ENABLE_NMODL`` defaults to **OFF**. The ``nmodl`` binary is still
  installed with CoreNEURON; use it explicitly for mechanism codegen (below).
- After ACC codegen changes to in-tree built-ins (e.g. ``expsyn``), force
  regenerate and reinstall: ``rm -f build-gpu/src/nrnoc/expsyn.cpp && ninja -C build-gpu install``.

Activate the install for runs
*****************************

Do **not** leave a prior ``nrnenv`` active during configure/build if your
environment exports ``N`` / ``PYTHONPATH`` into GNU make (it can break
``nrnivmodl-core`` makefiles). Activate only for simulation and ``nrnivmodl``:

.. code-block:: bash

   source ~/neuron/bin/nrnenv nrngpu build-gpu   # example helper; or set PATH/PYTHONPATH/LD_LIBRARY_PATH by hand
   export NRN_GPU_BACKEND_TEST=native
   export NRN_GPU_PERMUTE=2
   # Device nonvint is mandatory under native (no env switch).

Confirm a device is visible: ``nvidia-smi -L``. Successful native runs often log
``Info : 1 GPUs shared by 1 ranks per node``.

User mechanisms (NMODL + OpenACC)
********************************

GPU-capable mechanism libraries for the **NEURON** path need NMODL OpenACC
codegen (not plain NOCMODL). The shell ``nrnivmodl`` default on GPU installs
may still invoke NOCMODL for historical ctest reasons; pass ``-nmodl``
explicitly for device CURRENT / JACOBIAN / SOLVE registration (Gates B/C).

.. code-block:: bash

   source ~/neuron/bin/nrnenv nrngpu build-gpu
   mkdir -p /tmp/mymechs && cd /tmp/mymechs
   # link or copy your .mod files here
   nrnivmodl -nmodl "$(which nmodl)" \
     -nmodlflags "passes --inline host --c acc --oacc" .

Host-only NMODL (no OpenACC device kernels) omits ``-nmodlflags``:

.. code-block:: bash

   nrnivmodl -nmodl "$(which nmodl)" .

See also :doc:`/nmodl/gpu_codegen` for ``create_nrnmech`` / CMake workflows and
feature gaps.

**Install headers:** OpenACC-generated code includes
``neuron/model_data.hpp`` and related container headers. A GPU-enabled
``ninja install`` ships these under ``include/neuron/`` (and ``include/utils/logger.hpp``).
If a fresh configure omits them, rebuild after updating
``cmake/NeuronFileLists.cmake``.

**``-nocmodl``** is rejected on ``NRN_ENABLE_GPU`` builds.

Qualification and smoke tests
*****************************

Ringtest (long gate: 688 spikes @ ``tstop=100``, CPU vs native GPU
``prcellstate``)

.. code-block:: bash

   source ~/neuron/bin/nrnenv nrngpu build-gpu
   export NRN_GPU_BACKEND_TEST=native NRN_GPU_PERMUTE=2
   cd build-gpu/test/external_ringtest/neuron_gpu_native_mpi
   # After reinstall if needed:
   #   rm -rf x86_64 && nrnivmodl .
   ./prcellstate_native_gpu.sh 32 100

CTest (from the build directory):

.. code-block:: bash

   ctest -R 'external_ringtest::' --output-on-failure
   ctest -R gpu --output-on-failure
   ctest -R '_py_gpu_native' --output-on-failure

Models report fixed-step qualification with ``pc.gpu_fixed_step_phases()``
(HOC/Python). Certified native runs should show ``QUALIFIED: yes`` without
setting ``NRN_GPU_ALLOW_UNQUALIFIED``. Gate definitions live in
``doc/gpu-step-qualification.md``.

Traub-scale (ModelDB 82894) is the heavy threshold / mechanism stress case;
build mechs with the OpenACC ``nrnivmodl`` line above, then compare spike
counts and ``prcellstate`` against ``enable_gpu=0`` on the same install.

Optional local helper
*********************

Some workstations keep an untracked ``build-gpu/grok-bld`` script that encodes
the same CMake flags and NVHPC/OpenMPI paths for ``configure`` / ``build`` /
``test``. It is a convenience only; this page is the documented source of
truth for the recipe.

Related pages
*************

- :doc:`native-gpu-fixed-step` — scope contract and runtime API
- :doc:`gpu-testing` — CTest, MPI device assignment, ringtest ``-gpu-native``
- :doc:`/nmodl/gpu_codegen` — NMODL vs NOCMODL for GPU builds
- :ref:`cmake-nrn-enable-gpu-option` — CMake option details
- ``doc/gpu/threshold-detection.md`` — PreSyn threshold on device (Th0–Th3)
- ``doc/gpu-step-qualification.md`` — Gates A–F
