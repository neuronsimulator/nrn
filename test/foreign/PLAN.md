# Foreign ctest against PyPI wheels

**Branch:** `hines-grok/ctest-wheels`  
**Architecture:** separate test-only CMake project (option B)  
**Dev artifact:** normal venv + `pip install neuron-nightly`  
**v1 done:** documented local workflow (metric C)  
**Status:** M5 complete (hardening + inventory). Branch ready for review/park.

Approved product decisions:

| # | Decision |
|---|----------|
| 1 | Primary consumer: PyPI wheels; Windows setup.exe later backend |
| 2 | Version policy D: CI hard match; local soft skew needs `NRN_FOREIGN_ALLOW_SKEW` |
| 3 | Harness registers serial + MPI + CoreNEURON when available; CI/default serial first |
| 4 | Success metric C: local foreign-ctest workflow (not Azure replace) |
| 5 | Wheels first; develop against `pip install neuron-nightly` |

---

## Goals / non-goals

**Goals**

- Configure/run integration tests from the NEURON source tree against an already installed wheel.
- Discover features from the wheel (`neuron.config.arguments` + path probes).
- Version policy D as above.
- Harness can register serial + MPI + CoreNEURON when wheel/host support them.
- Windows `setup.exe` backend: design hook only in early milestones.

**Non-goals (early milestones)**

- Building `libnrniv` / Catch2 unit tests / NMODL unit libs in foreign mode.
- Replacing `test_wheels.sh` in Azure.
- Install-check as a milestone.
- GPU foreign suite.
- Root-monorepo `-DNRN_FOREIGN` on full NEURON build.

---

## Target UX (v1)

```bash
python -m venv .venv && source .venv/bin/activate
pip install -U pip neuron-nightly
# checkout matching revision, or pass -DNRN_FOREIGN_ALLOW_SKEW=ON
cmake -S test/foreign -B build-ctest -DNRN_FOREIGN_PYTHON="$(which python)"
cmake --build build-ctest --target foreign
ctest --test-dir build-ctest -j8
# later / local full features:
ctest --test-dir build-ctest -R 'mpi|coreneuron' -j8
```

---

## Design pillars

1. **Discovery** — `-DNRN_FOREIGN_PYTHON=`; probe script returns version, features, tool paths.
2. **Version gate** — CI (`NRN_FOREIGN_CI=ON`): mismatch fatal. Local: mismatch fatal unless `NRN_FOREIGN_ALLOW_SKEW=ON`.
3. **Features** — map wheel config (+ `mpiexec` probe) to existing `REQUIRES` flags in NeuronTestHelper.
4. **Environment** — no build-tree `NRN_RUN_FROM_BUILD_DIR_ENV`; use venv/wheel; only append test-data paths.
5. **Mechanisms** — foreign `nrnivmodl`, no `DEPENDS nrniv_lib`; target `foreign` builds mod jobs.

---

## File-level sketch

```text
test/foreign/                    # standalone CMake project root
  PLAN.md                        # this document
  CMakeLists.txt                 # M1+
  README.md                      # local workflow (metric C)
  DiscoverNeuron.cmake
  VersionGate.cmake
  probe_neuron.py
  ForeignTestHelpers.cmake

cmake/NeuronTestHelper.cmake     # parameterize nrnivmodl path, deps, RUN_ENV
test/CMakeLists.txt              # guards: linked units only if TARGET nrniv_lib
```

---

## Milestones

| ID | Content | Exit |
|----|---------|------|
| **M0** | Plan + branch `hines-grok/ctest-wheels` | Done |
| **M1** | Skeleton, discovery, version gate, smoke tests | Done (import / nrniv / neuron.test) |
| **M2** | Foreign nrnivmodl + one mod-using group; target `foreign` | Done (`pytest::basic_tests`) |
| **M3** | Serial portable subset + README (v1 done) | Done (~88 tests, `-L serial`) |
| **M4** | Register MPI + CoreNEURON for local `-R` | Done (`-L mpi`, `-L coreneuron`) |
| **M5** | Hardening, inventory, optional CI/prefix | Done (no Azure; prefix via `NRN_FOREIGN_ROOT`) |

### PR1 vertical slice

M0 + M1 (+ M2 if small). Review commands:

```bash
python -m venv /tmp/nrn-foreign-venv && source /tmp/nrn-foreign-venv/bin/activate
pip install neuron-nightly
cmake -S test/foreign -B /tmp/build-ctest \
  -DNRN_FOREIGN_PYTHON="$(which python)" \
  -DNRN_FOREIGN_ALLOW_SKEW=ON
cmake --build /tmp/build-ctest --target foreign
ctest --test-dir /tmp/build-ctest --output-on-failure
```

---

## Approval (2026-07-31)

- Entry point `test/foreign` — yes
- Soft skew requires explicit `NRN_FOREIGN_ALLOW_SKEW` — yes
- M1 smoke may be `neuron.test()` before nrnivmodl — yes
- M3 = v1 done (serial local); M4 = MPI/CN local — yes
- No Azure requirement in this plan — yes
- Wheels first; `pip install neuron-nightly` in a normal venv — yes
