# Foreign ctest (PyPI wheels)

Run NEURON integration tests from this source tree against an **already installed**
wheel (for example `neuron-nightly` in a virtual environment).

This is a **standalone** CMake project (`cmake -S test/foreign`). It does not build
`libnrniv`.

See [PLAN.md](PLAN.md) for milestones and design.

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

## Quick start (local, metric C — serial suite)

```bash
python -m venv .venv
source .venv/bin/activate
pip install -U pip neuron-nightly pytest matplotlib plotly anywidget

# From the NEURON repo root:
git submodule update --init -- test/rxd/testdata   # optional but needed for rxd

cmake -S test/foreign -B build-ctest \
  -DNRN_FOREIGN_PYTHON="$(which python)" \
  -DNRN_FOREIGN_ALLOW_SKEW=ON

cmake --build build-ctest --target foreign -j
ctest --test-dir build-ctest -L serial --output-on-failure -j4
```

`NRN_FOREIGN_ALLOW_SKEW=ON` is required when the wheel’s git revision does not
match this checkout (typical for `neuron-nightly` vs a feature branch).

### Useful filters

| Filter | Meaning |
|--------|---------|
| `-L serial` | Full M3 portable suite (default “done” set) |
| `-L smoke` | Import / nrniv / `neuron.test()` only |
| `-L pytest` / `-L hoctests` / `-L rxd` / … | Subsets by group label |
| `-R 'pytest::\|datahandle::'` | Regex on test names |

MPI and CoreNEURON integration groups are deferred to a later milestone (M4);
feature discovery already records whether the wheel has them.

## Version policy

| Mode | CMake options | Mismatch |
|------|----------------|----------|
| Local strict (default) | (none) | Configure **fails** |
| Local exploration | `-DNRN_FOREIGN_ALLOW_SKEW=ON` | Configure **warns**, continues |
| CI | `-DNRN_FOREIGN_CI=ON` | Configure **fails** (hard match; leave `ALLOW_SKEW` off) |

Match prefers the wheel’s build git SHA (`h.nrnversion(3)`) against this repo’s
`HEAD`, then equal version/`git describe` strings.

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
| `gjtests` serial | pytest `-k "not par"` |

**Excluded:** Catch2 / API / NMODL unit binaries, MPI tests, CoreNEURON comparison matrix (M4).

Target `foreign` builds all registered `nrnivmodl` jobs and copies test scripts.

## Layout

```text
test/foreign/
  CMakeLists.txt              # project entry
  DiscoverNeuron.cmake
  VersionGate.cmake
  ForeignTestHelpers.cmake    # NeuronTestHelper foreign adapter
  SerialPortableTests.cmake   # M3 serial groups
  probe_neuron.py
  PLAN.md
  README.md
```
