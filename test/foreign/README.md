# Foreign ctest (PyPI wheels)

Run NEURON integration tests from this source tree against an **already installed**
wheel (for example `neuron-nightly` in a virtual environment).

This is a **standalone** CMake project (`cmake -S test/foreign`). It does not build
`libnrniv`.

## Prerequisites

- CMake ≥ 3.20
- A C/C++ toolchain on `PATH` (for `nrnivmodl`)
- A venv with NEURON, e.g. `pip install neuron-nightly`
- Python packages for the serial suite:
  - **required for most groups:** `pytest`
  - **required for full RxD:** `matplotlib`, `plotly`, `anywidget`
- Prefer a clean shell for the venv: an ambient `PYTHONPATH` pointing at a
  source build can shadow the wheel. Configure/tests replace `PYTHONPATH`
  with in-tree helpers only (`test/rxd`).

Optional:

```bash
git submodule update --init -- test/rxd/testdata   # for RxD comparison data
```

See [INVENTORY.md](INVENTORY.md) for foreign-ok vs build-only tests and skip rules.

## Quick start (local, metric C — serial suite)

```bash
python -m venv .venv
source .venv/bin/activate
pip install -U pip neuron-nightly pytest matplotlib plotly anywidget mpi4py

# From the NEURON repo root:
git submodule update --init -- test/rxd/testdata   # optional but needed for rxd

cmake -S test/foreign -B build-ctest \
  -DNRN_FOREIGN_PYTHON="$(which python)" \
  -DNRN_FOREIGN_ALLOW_SKEW=ON

# Default serial install check (builds mechanisms, runs ctest -L serial, prints
# how to re-run ctest with other filters):
cmake --build build-ctest --target test-install -j
# same: ninja -C build-ctest test-install

# Or split prep and ctest yourself:
cmake --build build-ctest --target foreign -j
ctest --test-dir build-ctest -L serial --output-on-failure -j4

# When the wheel has MPI / CoreNEURON and mpiexec is on PATH:
ctest --test-dir build-ctest -L mpi --output-on-failure -j2
ctest --test-dir build-ctest -L coreneuron --output-on-failure -j2
```

`test-install` is a convenience default only. Full `ctest` control is always:

```bash
ctest --test-dir build-ctest [options…]
```

Cache knobs (reconfigure to change defaults):

| Variable | Default | Meaning |
|----------|---------|---------|
| `NRN_FOREIGN_TEST_INSTALL_LABELS` | `serial` | `-L` for `test-install` |
| `NRN_FOREIGN_TEST_INSTALL_JOBS` | `4` | `-j` for `test-install` |

`NRN_FOREIGN_ALLOW_SKEW=ON` is required when the wheel’s git revision does not
match this checkout (typical for `neuron-nightly` vs a feature branch).

### Useful filters

| Filter | Meaning |
|--------|---------|
| `-L serial` | Full M3 portable suite (metric C “done” set) |
| `-L smoke` | Import / nrniv / `neuron.test()` only |
| `-L mpi` | MPI smoke + parallel + CN+MPI (needs `mpiexec` at configure) |
| `-L coreneuron` | CoreNEURON CPU suite (skipped if wheel has no CN) |
| `-L pytest` / `-L hoctests` / `-L rxd` / … | Subsets by group label |
| `-R 'pytest::\|datahandle::'` | Regex on test names |

If configure cannot find `mpiexec`, MPI feature is treated as off and no `-L mpi`
tests are registered (clean skip, not failure).

## Version policy

| Mode | CMake options | Mismatch |
|------|----------------|----------|
| Local strict (default) | (none) | Configure **fails** |
| Local exploration | `-DNRN_FOREIGN_ALLOW_SKEW=ON` | Configure **warns**, continues |
| CI | `-DNRN_FOREIGN_CI=ON` | Configure **fails** (hard match; leave `ALLOW_SKEW` off) |

Match rules (`VersionGate.cmake`):

1. Normalized git SHA (flexible length; also extracted from describe strings like
   `9.0.1-85-g5ac449d89` used by nightlies)
2. Exact version / `git describe` string equality

## Prefix install (optional)

Wheels are primary. For a classic `CMAKE_INSTALL_PREFIX` layout you can either
use the **main NEURON build** helper target or configure foreign yourself.

### From a normal NEURON build (`-DNRN_ENABLE_TESTS=ON`)

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_INSTALL_PREFIX=$PWD/build/install \
  -DNRN_ENABLE_TESTS=ON
cmake --build build -j
cmake --build build --target install
cmake --build build --target test-install
# or from the build directory:
#   ninja install
#   ninja test-install
```

That creates **`${CMAKE_BINARY_DIR}/build-ctest`** (e.g. `build/build-ctest`),
configures `test/foreign` against the install prefix, builds mechanisms, and
runs the default serial foreign suite. It does **not** exist until you run
`test-install` once (or configure foreign yourself into that path).

Then filter with plain ctest on that dir:

```bash
ctest --test-dir build/build-ctest -L mpi --output-on-failure
```

### Manual foreign configure against a prefix

```bash
cmake -S test/foreign -B build-ctest \
  -DNRN_FOREIGN_PYTHON=/path/to/python \
  -DNRN_FOREIGN_ROOT=/path/to/prefix \
  -DNRN_FOREIGN_ALLOW_SKEW=ON   # only if tree ≠ install revision
cmake --build build-ctest --target test-install
```

`NRN_FOREIGN_ROOT/bin` is prepended for tools; `NRN_FOREIGN_ROOT/lib/python` is
put on `PYTHONPATH` so `import neuron` works for the default install layout.

## Discovery

Configure runs `probe_neuron.py` with `NRN_FOREIGN_PYTHON` and writes
`build-ctest/foreign_neuron_probe.json` (version, features, tool paths).

Useful cache variables (see `cmake -L` / `CMakeCache.txt`):

- `NRN_FOREIGN_PYTHON`
- `NRN_FOREIGN_NEURON_VERSION_FULL`
- `NRN_FOREIGN_NRNIV`, `NRN_FOREIGN_NRNIVMODL`
- `NRN_FOREIGN_FEATURE_NRN_ENABLE_MPI`, `NRN_FOREIGN_FEATURE_NRN_ENABLE_CORENEURON`, …

## What is included (M3 serial)

| Area | Notes |
|------|--------|
| Smoke | `foreign::smoke_*` |
| `pytest`, `datahandle`, `cover` | pytest + nrnivmodl |
| `unit_tests` (hoc_python) | no mods |
| `example_nmodl` | HOC via `special`, Python via pytest |
| `hoctests` | HOC via `special`, Python via plain interpreter |
| `ringtest`, `connect_dend` | foreign `nrniv` + `RunHOCTest.cmake` |
| `rxdmod_tests` | if wheel has RX3D and testdata submodule present |
| `gjtests` serial | python `test_natrans.py` (in-tree script; `test_par_gj.py` is `gj_par`) |

### What is included (M4 — MPI / CoreNEURON)

| Area | Labels | Notes |
|------|--------|--------|
| `mpi_init` | `mpi` | nrniv/python MPI smoke + mpiexec |
| `parallel` | `mpi` | bas, partrans, netpar, subworld, nrntest_fast |
| `pytest_coreneuron` | `coreneuron` | serial pytest with CN-enabled special |
| `coreneuron_standalone` / `coreneuron_modtests` | `coreneuron` | CPU direct/spikes/psolve/… + `inputpresyn` MPI |
| `gjtests::gj_par` | `mpi` | multi-rank gap junction |

**Excluded:** Catch2 / API / NMODL unit binaries, GPU CoreNEURON matrix.

Target `foreign` builds all registered `nrnivmodl` jobs (including `-coreneuron`) and copies test scripts.

## Layout

```text
test/foreign/
  CMakeLists.txt              # project entry
  DiscoverNeuron.cmake
  VersionGate.cmake
  ForeignTestHelpers.cmake    # NeuronTestHelper foreign adapter
  SerialPortableTests.cmake   # M3 serial groups
  MpiCoreNeuronTests.cmake    # M4 MPI + CoreNEURON
  RunTestInstall.cmake        # body of test-install target
  probe_neuron.py
  INVENTORY.md                # foreign-ok vs build-only
  README.md
```
