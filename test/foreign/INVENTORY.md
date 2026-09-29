# Foreign ctest inventory

Which integration tests the **foreign** (wheel) harness can run vs what still
requires an in-tree NEURON build.

Last updated with M5. Labels refer to `ctest -L …` on a `test/foreign` build.

## Foreign-ok (registered by `test/foreign`)

| Area | Labels | Needs |
|------|--------|--------|
| Smoke import / nrniv / `neuron.test()` | `smoke`, `serial` | wheel + PATH |
| `pytest` (mcna) | `pytest`, `serial` | `pytest`, nrnivmodl |
| `datahandle` | `datahandle`, `serial` | `pytest`, nrnivmodl |
| `coverage_tests` / cover | `cover`, `serial` | `pytest`, nrnivmodl |
| `unit_tests` hoc_python | `unit`, `serial` | `pytest` |
| `example_nmodl` | `example_nmodl`, `serial` | nrnivmodl / special |
| `hoctests` | `hoctests`, `serial` | nrnivmodl / special |
| `ringtest`, `connect_dend` | `ringtest` / `connect_dend`, `serial` | nrniv |
| `rxdmod_tests` | `rxd`, `serial` | RX3D wheel, `test/rxd/testdata`, `pytest`, optional matplotlib/plotly/anywidget |
| `gjtests` serial | `gjtests`, `serial` | `pytest`, nrnivmodl |
| `mpi_init` | `mpi` | MPI wheel + `mpiexec` |
| `parallel` | `mpi`, `parallel` | MPI + `mpiexec` + nrnivmodl |
| `pytest_coreneuron` | `coreneuron`, `serial` | CoreNEURON wheel, `pytest` |
| `coreneuron_*` CPU | `coreneuron` | CoreNEURON + nrnivmodl `-coreneuron` |
| CN + MPI (`inputpresyn`, …) | `mpi`, `coreneuron` | both |
| `gjtests::gj_par` | `mpi` | MPI + `mpiexec` |

### Clean skip (not failure)

| Condition | Effect |
|-----------|--------|
| No `mpiexec` at configure | MPI treated as off; no `-L mpi` tests |
| Wheel without CoreNEURON | No `-L coreneuron` tests |
| Wheel without RX3D / empty testdata | No `rxdmod_tests` |
| No `pytest` | Python pytest groups skipped (warning) |
| GPU off in wheel | No GPU CoreNEURON variants (not registered) |

## Build-only (not foreign)

These need a full NEURON CMake build (`libnrniv`, Catch2, etc.):

| Area | Why build-only |
|------|----------------|
| `testneuron` / container Catch2 units | Links `nrniv_lib` |
| API C++ tests (`test/api`) | Links public C API / internals |
| CoreNEURON C++ unit tests | Links CoreNEURON unit libraries |
| NMODL transpiler unit / integration binaries | Builds against NMODL libs / `nmodl` target |
| Sanitizer / coverage instrumented binaries | Build-time instrumentation |
| MUSIC tests | Rare external dep; not wired for foreign |
| External model submodule suites (beyond rxd data) | Often assume full build env |
| `nrnivmodl_cmake` install-check against *this* build tree | Different goal (install layout of the tree under test) |
| pyinit multi-venv matrix | Assumes multiple build-configured Pythons |

## Version policy (quick)

| Mode | Behavior |
|------|----------|
| Default local | Mismatch → configure **fails** |
| `-DNRN_FOREIGN_ALLOW_SKEW=ON` | Mismatch → **warn**, continue |
| `-DNRN_FOREIGN_CI=ON` | Mismatch → **fails** (hard match) |

Match rules (see `VersionGate.cmake`): git SHA (flexible length / embed in describe),
then full version string equality.

## Optional prefix install

Wheels remain primary. A classic prefix works if:

```bash
export PATH=/path/to/prefix/bin:$PATH
cmake -S test/foreign -B build-ctest \
  -DNRN_FOREIGN_PYTHON="$(which python3)" \
  -DNRN_FOREIGN_ROOT=/path/to/prefix \
  -DNRN_FOREIGN_ALLOW_SKEW=ON   # if needed
```

`NRN_FOREIGN_ROOT` prepends `bin/` for discovery and test PATH (prefix backend).
