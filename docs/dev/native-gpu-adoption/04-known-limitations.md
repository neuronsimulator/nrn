# Known limitations (Phase B)

Runtime limits documented in the Phase B contract. These are **not bugs to fix
in Phase B**; they define the boundary.

## CVode / IDA

Native GPU is fixed-step only. Enabling native GPU with CVode active raises an
error (`native_gpu_configuration_error()`). Use CPU NEURON or CoreNEURON for
variable-step models.

## GPU spike priority queue

Cross-rank spike scheduling remains on CPU with MPI. There is no device-resident
priority queue equivalent to the host NetCon queues.

## Multi-thread OpenACC

Validated default: `pc.nthread(1)`, `OMP_NUM_THREADS=1`. `pc.nthread(n>1)`
emits a one-time warning; per-thread CUDA contexts may fail on some platforms.

## sparse13 / DAE models (extracellular, LinearMechanism)

Native fixed-step is implemented as **separated GPU subphases**. For ODE tree
models, **GPU tree solve** (`solve_interleaved`) is on device. Extracellular and
`LinearMechanism` force `use_sparse13 = 1` and require **CPU sparse13 Gaussian
elimination** — the one subphase with no GPU implementation in Phase B.

Other subphases (mechanism currents, `NET_RECEIVE`, post-solve in principle) are
not ruled out by the architecture, but Phase B does not certify DAE models on
`gpu.backend="native"`. `post_solve_needs_host_fallback()` often forces CPU
post-solve for DAE/LFP cases as well.

## LFP / partrans post-solve fallback

When `nrnthread_vi_compute_` is registered (gap/partrans v+vext interpolation),
post-solve runs on the host via `nrn_update_voltage` instead of
`post_solve_on_device`. The matrix solve may still be on GPU for type-1 ODE models.

## MPI-native modtest gap

The 19-test parity set is single-process. MPI-native expansion (`spikes_mpi`,
`test_subworlds`, etc.) is not part of Phase B verification.

## CMake: CoreNEURON required at configure

`-DNRN_ENABLE_GPU=ON` without `-DNRN_ENABLE_CORENEURON=ON` fails configure.
This is a **build-system** limitation, not a runtime simulation limit. Native
simulations still avoid `coreneuron.enable=True`. Standalone GPU configure is
deferred (see [05-future-work.md](05-future-work.md)).

## `coreneuron.enable` wins over `gpu.backend`

If both `coreneuron.enable=True` and `gpu.backend="native"`, `pc.psolve`
embeds CoreNEURON — it does **not** run the native path. See the runtime table
in [`native-gpu-fixed-step.rst`](../native-gpu-fixed-step.rst).