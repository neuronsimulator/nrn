# Foreign ctest (PyPI wheels)

Run NEURON integration tests from this source tree against an **already installed**
wheel (for example `neuron-nightly` in a virtual environment).

This is a **standalone** CMake project (`cmake -S test/foreign`). It does not build
`libnrniv`.

See [PLAN.md](PLAN.md) for milestones and design.

## Prerequisites

- CMake ≥ 3.20
- A venv (or environment) with NEURON installed, e.g. `pip install neuron-nightly`
- That environment’s `python` and scripts (`nrniv`, `nrnivmodl`) on `PATH` when
  configuring and running tests
- Prefer a clean shell for the venv: an ambient `PYTHONPATH` pointing at a
  source build can shadow the wheel. Configure/tests clear `PYTHONPATH` for
  the probe and smoke tests, but interactive debugging is easier without it.

## Quick start (local, metric C)

```bash
python -m venv .venv
source .venv/bin/activate
pip install -U pip neuron-nightly

# From the NEURON repo root (this tree):
cmake -S test/foreign -B build-ctest \
  -DNRN_FOREIGN_PYTHON="$(which python)" \
  -DNRN_FOREIGN_ALLOW_SKEW=ON

cmake --build build-ctest --target foreign
ctest --test-dir build-ctest --output-on-failure -L smoke
```

`NRN_FOREIGN_ALLOW_SKEW=ON` is required when the wheel’s git revision does not
match this checkout (typical for `neuron-nightly` vs a feature branch).

## Version policy

| Mode | CMake options | Mismatch |
|------|----------------|----------|
| Local strict (default) | (none) | Configure **fails** |
| Local exploration | `-DNRN_FOREIGN_ALLOW_SKEW=ON` | Configure **warns**, continues |
| CI | `-DNRN_FOREIGN_CI=ON` | Configure **fails** (hard match; skew flag does not apply as a bypass in CI intent—leave `ALLOW_SKEW` off) |

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

## Current status (M1)

- Discovery + version gate
- Smoke tests: `foreign::smoke_import`, `foreign::smoke_nrniv`, `foreign::smoke_neuron_test`
- Target `foreign` is a no-op placeholder (mechanism builds in M2+)

## Next (M2+)

- Foreign `nrnivmodl` and portable integration groups
- MPI / CoreNEURON registration for local `ctest -R …`
