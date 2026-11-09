# NEURON test directory

| File or directory | Contents |
| --- | --- |
| [CMakeLists.txt](CMakeLists.txt) | Controls how all tests are compiled and run. |
| [api/](api/) | C++ tests of the NEURON C API, including sections, mechanisms, HOC objects and symbols, pointers, simulation, and error handling. |
| [benchmarks/](benchmarks/) | Multicore correctness and performance tests comparing serial, worker-thread, and busy-wait execution. |
| [common/](common/) | Shared Catch2 test runner that initializes NEURON for C++ unit tests and benchmarks. |
| [coreneuron/](coreneuron/) | CoreNEURON integration tests for data transfer, events, spikes, pointers, random streams, and execution modes; also contains engine unit tests under `unit/`. |
| [cover/](cover/) | Tests targeting code-coverage gaps, especially CVode and network-event paths, plus small C++ regression cases. |
| [datahandle/](datahandle/) | Tests of variable references and data handles across storage changes, deleted sections, opaque pointers, and mechanism access. |
| [external/](external/) | CMake setup for tests and model benchmarks fetched from external repositories, including nrntest, ring networks, and NEURON/CoreNEURON comparisons. |
| [foreign/](foreign/) | Standalone CMake harness for running this checkout's integration tests against an installed NEURON wheel or prefix; see its [README](foreign/README.md). |
| [gjtests/](gjtests/) | Gap-junction and continuous-variable transfer tests using ParallelContext, including MPI and multithreaded cases. |
| [hoc_tests/](hoc_tests/) | Small HOC regression model connecting dendrites to a soma and comparing recorded voltage with reference data. |
| [hoctests/](hoctests/) | HOC and Python scripts covering interpreter behavior, simulator objects, mechanisms, GUI helpers, and morphology; `vardimtests/` checks variable array dimensions. |
| [music_tests/](music_tests/) | MUSIC coupling tests for exchanging events between simulator processes, with launch configurations and a Python runner. |
| [nmodl/](nmodl/) | Mechanism-language tests for tables, kinetics, randomness, and paths with spaces; `transpiler/` contains compiler unit tests, integration tests, and language use cases. |
| [nrnivmodl_cmake/](nrnivmodl_cmake/) | Tests of compiling MOD mechanisms through NEURON's exported CMake package and `create_nrnmech` helper. |
| [parallel_tests/](parallel_tests/) | MPI network tests for saving/restoring distributed simulations and ParallelContext subworlds and bulletin-board tasks. |
| [pyinit/](pyinit/) | Python startup and embedding tests: script and command execution, arguments, exit codes, interpreter selection, and Python environment consistency. |
| [pytest/](pytest/) | NEURON-only pytest cases, including threaded mechanism execution and LinearMechanism callbacks. |
| [pytest_coreneuron/](pytest_coreneuron/) | Pytest cases compatible with both NEURON and CoreNEURON, covering Python/HOC interfaces, vectors, morphology, currents, state saving, and parallel simulation. |
| [ringtest/](ringtest/) | Small HOC ring network of ball-and-stick cells, with reference spike output for regression comparisons. |
| [rxd/](rxd/) | Reaction-diffusion tests for species, reactions, diffusion, currents, fluxes, and saved state, including intracellular 3D, extracellular, and hybrid geometries. |
| [scripts/](scripts/) | Utilities for comparing simulation output, currently ASCII spike results across NEURON and CoreNEURON runs. |
| [unit_tests/](unit_tests/) | C++ unit tests of simulator internals, containers, numerical operations, node ordering, HOC, and utilities, plus Python/HOC interface and cell-state comparison tests. |

## Tests outside this directory

| Location | Contents |
| --- | --- |
| [share/lib/python/neuron/tests/](../share/lib/python/neuron/tests/) | Unit tests shipped inside the `neuron` Python package and run by `neuron.test()` and `neuron.test_rxd()`, covering the Python bindings, Vector, NMODL, and RxD. |
| [share/lib/python/neuron/rxdtests/](../share/lib/python/neuron/rxdtests/) | RxD regression scripts; `run_all.py` runs each script in `tests/` and compares its output with reference data in `rxd/testdata/`. |
| [packaging/python/test_wheels.sh](../packaging/python/test_wheels.sh) | Smoke tests for a built Python wheel, in serial and with MPI. |
| [src/parallel/](../src/parallel/) | Small `test*.hoc` scripts for ParallelContext and bulletin-board tasks, run by hand. |
