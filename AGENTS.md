# AGENTS.md

Instructions for AI coding agents working on NEURON. Humans: everything here is
also true for you, and [CONTRIBUTING.md](CONTRIBUTING.md) is the full guide.

NEURON is a simulator for neurons and networks of neurons, developed since the
1980s. Thousands of published models depend on it behaving the same way it did
when they were published. Most of the people who review your
work do not use AI tools, and many are scientists rather than software
engineers. Write for them.

## The person you work for is the author

- A human submits the work and answers for every line of it. Do not push, open
  pull requests, or post issues, comments or reviews on GitHub unless that person
  asks you to in this session.
- That human must read and review every edit you make by hand before it is
  submitted, because they are responsible for it. Keep your changes small and
  clear enough that this review is practical.
- Never say a build or test passed unless you ran it and saw it pass. If you
  could not run something, say so.
- When a request is ambiguous on a scientific point (units, which equation, what
  a model should do), stop and ask. Do not guess.

## Reproducibility of published results

Existing models must keep giving the same qualitative results.

- Do not change the formulas being evaluated or the algorithms used to evaluate
  them unless the human explicitly asks for it. This includes numerical methods
  (CVODE/IDA settings, the fixed-step scheme, `sparse13`, `scopmath`, NMODL code
  generation).
- Results need not be bit-for-bit identical: changes at the level of
  floating-point rounding are acceptable. If your change can alter results at
  all, for example by reordering arithmetic, say so in the commit message and in
  your report.
- Never edit reference data just to make a test pass. This includes the `*.json`
  files read by `neuron.tests.utils.checkresult.Chk`, the `REFERENCE_OUTPUT`
  files of `nrn_add_test_group_comparison`, and the `*.ref`, `*.std` and `*.spk`
  files in test repositories. If a reference must change, explain why and get
  the human's agreement. `Chk` silently adds a key that is missing, so check that
  new keys hold values you have verified independently.
- Never loosen a tolerance, skip, disable or delete a test to get green.
- Keep HOC, NMODL and Python behaviour backward compatible. Old `.mod` files and
  HOC scripts must keep working and keep giving the same results. Removing or
  changing a user-visible feature needs a maintainer's agreement, an entry in
  `docs/changelog.md`, and, for removals, a row in `docs/removed_features.rst`.

## Scientific correctness

- Every new equation, algorithm or constant needs a source: a paper, a textbook,
  or a derivation in the comment. Give physical units for quantities (NEURON's
  conventions are mV, ms, µm, mA/cm² and S/cm² for density mechanisms, nA and
  µS for point processes, mM).
- Tests must be able to fail. Prefer checks against an analytic solution, a
  conservation law, or an independent implementation over "it runs" or "it
  matches what the code printed today".

## Readability and review

- Make the smallest change that does the job. Match the style, naming and
  comment density of the surrounding code, even where it is old.
- No drive-by reformatting, renaming or "modernising" of code you were not asked
  to change. It hides the real change from reviewers and from `git blame`.
- Put behaviour-neutral refactors and behaviour changes in separate commits.
- Write commit messages in plain language: what changed, why, and how it was
  tested. Assume the reader knows the science but not the C++.
- Comments explain why, and the science. Do not leave comments about the tools
  used to write the code (for example "suggested by an AI"). Do not leave
  comments that only make sense in the context of a now-fixed bug.
- Every commit you helped write ends with an `Assisted-by: <tool> (<model>)`
  line that names the tool and the model it used.

## Layout

- `src/nrnoc`, `src/nrniv`, `src/nrncvode`: simulation core, fixed-step and
  variable-step integration.
- `src/oc`: the HOC interpreter. `src/nrnpython`: the Python binding.
  `share/lib/python/neuron`: the `neuron` Python package, including `rxd`.
- `src/nocmodl` (default) and `src/nmodl`: translators from NMODL `.mod` files
  to C++. `src/coreneuron`: the optimised engine for large and GPU runs.
- `src/sundials`, `src/sparse13`, `src/scopmath`, `src/pocketfft`: third-party
  numerical code, some of it locally modified. Change only with a clear reason.
- `external/`: git submodules. Do not edit them here.
- `docs/progref`: the user-facing reference. Update it when user-visible
  behaviour changes. `docs/dev`: notes on internals.

## Build and test

```sh
mkdir build && cd build
cmake .. -DNRN_ENABLE_TESTS=ON -DNRN_ENABLE_INTERVIEWS=OFF \
         -DNRN_ENABLE_MPI=OFF -DNRN_ENABLE_RX3D=OFF \
         -DPYTHON_EXECUTABLE=$(which python3)   # a Python with pytest
cmake --build . --parallel
ctest --output-on-failure -j8          # or: ctest -R <regex>
```

- `NRN_ENABLE_TESTS` is off by default; without it `ctest` finds no tests. The
  `test` target does not rebuild first.
- A HOC or Python test placed in a subdirectory of `test/hoctests/` (usually
  `tests/`) is picked up automatically after re-running CMake. C++ unit tests use Catch2
  (`test/unit_tests`, `test/api`).
- Formatting: `cmake --build . --target format-pr` (clang-format, cmake-format,
  black). Format only the files you changed.
