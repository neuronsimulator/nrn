# Appendix: commits

Chronological log for `hines-grok/feature/neuron-core-gpu-adoption` (oldest first).
One paragraph per commit through PR7 (`ab4efc1fe`).

---

### `d53a3c17b`

**Subject:** Add NRN_ENABLE_GPU and NRN_GPU_BACKEND CMake options (GPU adoption step 1)

Introduces the user-facing `NRN_ENABLE_GPU` CMake option and `NRN_GPU_BACKEND=OpenACC`
scaffolding. Establishes L0 build switches separate from CoreNEURON's
`CORENRN_ENABLE_GPU`, with Phase A transition guard requiring CoreNEURON when
GPU is enabled.

### `270919afa`

**Subject:** GPU adoption step 1b: document CMAKE_CUDA_ARCHITECTURES in options.rst

Documents optional `CMAKE_CUDA_ARCHITECTURES` for workstation GPU builds in
`docs/cmake_doc/options.rst`, reducing over-compilation when targeting a single
NVIDIA architecture.

### `1ef4465f5`

**Subject:** GPU adoption step 2: fix ringtest GPU MPI ctest NVHPC loader issue

Fixes `external_ringtest::coreneuron_gpu_mpi` failures from NVHPC
`pgcudafat` loader issues by routing the MPI ringtest through `special-core`
(dump with `special`, simulate with `special-core --gpu`).

### `bed18a476`

**Subject:** GPU adoption step 3: propagate OpenACC compile+link to libnrniv

Wires CoreNEURON-exported OpenACC compile and link flags into `libnrniv` and
entry points so NEURON proper can host OpenACC object code alongside classic CPU
sources.

### `4d94ca7f0`

**Subject:** GPU adoption step 4: add neuron::gpu offload scaffold

Adds `src/neuron/gpu/` module with `neuron_gpu` static library and OpenACC
offload helpers — the foundation for L2–L4 native GPU code.

### `92286d995`

**Subject:** GPU adoption step 5: device_token lifecycle and NrnThread GPU fields

Defines `device_token` and extends `NrnThread` with GPU bookkeeping fields so
upload state can be cached and invalidated with model structure changes.

### `3101e4faf`

**Subject:** GPU adoption step 6: default NMODL_NEURON_CODEGEN for GPU builds

Sets default NMODL NEURON codegen for GPU builds so mechanism MOD files generate
OpenACC-capable C++ for the native path, not only CoreNEURON targets.

### `847b56560`

**Subject:** GPU adoption step 7: enable solve_interleaved GPU dispatch scaffolding

Connects interleaved matrix solve dispatch to GPU when permutation is active —
prerequisite for L4 integration on device.

### `688144b72`

**Subject:** GPU adoption step 8: NMODL NEURON OpenACC codegen visitor

Implements the NMODL visitor that emits NEURON-side OpenACC mechanism code,
parallel to CoreNEURON's GPU codegen path.

### `233d5173d`

**Subject:** Fix step 8 file headers: drop Blue Brain copyright, EOF newlines

Housekeeping on step-8 NMODL sources: copyright headers and EOF newlines for
NEURON repo policy compliance.

### `47ed2db60`

**Subject:** GPU adoption step 9: SOA and InterleaveInfo device upload (MVP)

Minimum viable upload of SOA layout and `InterleaveInfo` to device — first
end-to-end data present for GPU timestep experiments.

### `7a1dc70fb`

**Subject:** GPU adoption step 10: native psolve dispatch scaffolding

Routes `nrn_fixed_step_thread` to `fadvance_gpu` when native backend is enabled
(L3 dispatch scaffolding).

### `95c28200f`

**Subject:** GPU adoption step 11: host-side network event stages (Phase B1)

Adds host-side network event delivery stages before/after the GPU integration
step, preserving CPU spike queue semantics during native GPU steps.

### `3a6955d05`

**Subject:** GPU adoption step 12: end-to-end native psolve scaffolding (G4 subset)

First end-to-end native `pc.psolve` for a G4 modtest subset — proves the
scaffolded path can complete a simulation.

### `0ba3acef1`

**Subject:** GPU adoption step 13: ringtest native GPU benchmark harness

Adds `-gpu-native` ringtest support and CTest `neuron_gpu_native_mpi` for
benchmarking the native path against reference spike files.

### `0100ec7d9`

**Subject:** GPU adoption step 14: MPI multi-GPU device assignment

Implements `device_id = mpi_local_rank % num_gpus_per_node` policy for native
GPU, matching CoreNEURON `init_gpu()` behavior.

### `94e689da3`

**Subject:** GPU adoption step 15: Python GPU API consolidation + MKL guard

Introduces `neuron.gpu` module with HOC sync; deprecates `coreneuron.gpu` shims.
Adds Intel OpenMP/MKL import guard for NVHPC OpenACC compatibility.

### `2663a5c50`

**Subject:** GPU adoption step 16: test stabilization (ringtest spk2, backend_helper copies)

Stabilizes ringtest spike comparisons and `backend_helper.py` state copies between
native and CoreNEURON test paths.

### `0dd2d055b`

**Subject:** GPU adoption PR 7b: OpenACC setup_tree_matrix in treeset.cpp

OpenACC offload of `setup_tree_matrix` for the native path — moves axial matrix
assembly work toward device execution.

### `a225a10f0`

**Subject:** GPU adoption PR 11b+16: net_send GPU buffer + gap/VecPlay sync

Adds `NetSendBuffer` for device-generated `net_send` events and gap junction /
VecPlay device↔host sync hooks (L6 communication).

### `a6c488e0f`

**Subject:** Fix GPU test failures: upload parent index, hybrid host/device sync

Fixes parent index upload and hybrid sync bugs uncovered by expanding modtests —
correctness fixes before parity expansion.

### `8795cd51e`

**Subject:** Fix native GPU fast_imem sync for pointer modtests

Ensures `fast_imem` device state is visible where pointer MOD mechanisms read
it — unblocks pointer-related native GPU tests.

### `41ce00183`

**Subject:** Enable GPU axial matrix assembly and GPU solve_interleaved on native path

Completes L4 integration: GPU axial matrix assembly and interleaved solve on the
native fixed-step path.

### `b6bad25dd`

**Subject:** Run GPU post-solve voltage update and fast_imem on device

Moves post-solve voltage update, `fast_imem`, and capacity current to device
(`post_solve_on_device`), reducing per-step host pulls.

### `574b22a2f`

**Subject:** Upload padded Memb_list shells for native GPU mechanism path

Uploads padded `Memb_list` layouts for OpenACC mechanism kernels — device SOA
parity with CoreNEURON assumptions.

### `e0037c448`

**Subject:** Add batch GPU download API for Vector.record and graph flush

Adds `download_flush_interval` and batch host download for recordings and HOC
graph reads (L7).

### `e2eddbf30`

**Subject:** Expand G4 native GPU modtests to full _py_gpu parity (PR 19)

Expands native modtests to **19** `*_py_gpu_native` targets mirroring
single-process `*_py_gpu` — Phase B L8 verification anchor.

### `ff48b5e78`

**Subject:** Reject native GPU when CVode is active (Phase B boundary)

Adds `native_gpu_configuration_error()` at Python, HOC, and `nrn_fixed_step`
entry — declares CVode out of scope for native GPU.

### `b5fdf7a79`

**Subject:** Stabilize GPU ctest harness for pytest modtests (Phase B)

Sets `LD_LIBRARY_PATH` for pytest modtests; relinks `libnrnmech` when
`libnrniv.so` changes; updates `gpu-testing.rst` test counts.

### `e800b0ffc`

**Subject:** Document native GPU threading and spike/NET_RECEIVE policy (Phase B)

One-time `pc.nthread(n>1)` warning; documents spike/`NET_RECEIVE` split in
`gpu-testing.rst`, `net_events.hpp`, `net_send_buffer.hpp`.

### `8fe50fa88`

**Subject:** Add native GPU fixed-step Sphinx scope contract (PR6 / Phase B)

Adds `docs/dev/native-gpu-fixed-step.rst` as canonical Phase B reference with
L0–L8 layers, build/runtime tables, mermaid timestep flow, and API docs.
Dedupes scope text from `gpu-testing.rst`; cross-links CMake options.

### `ab4efc1fe`

**Subject:** Add native GPU adoption dev journal and Phase B closure (PR7)

Adds `docs/dev/native-gpu-adoption/` development journal (overview through
commit appendix, `PHASE-B-COMPLETE` checklist). Links from dev index and Sphinx
scope page. Closes Phase B documentation scope without L0 build decouple work.