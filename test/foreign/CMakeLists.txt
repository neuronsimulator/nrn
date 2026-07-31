# Standalone project: run NEURON integration tests against a foreign (wheel) install.
# See PLAN.md and README.md.
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(nrn-foreign-ctest LANGUAGES NONE)

include(CTest)
if(NOT BUILD_TESTING)
  message(FATAL_ERROR "BUILD_TESTING is OFF; foreign ctest requires testing to be enabled")
endif()

# Repo root (nrn/) — this file lives at test/foreign/CMakeLists.txt
get_filename_component(NRN_FOREIGN_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(NRN_FOREIGN_SOURCE_ROOT
    "${NRN_FOREIGN_SOURCE_ROOT}"
    CACHE PATH "NEURON source tree root used for tests and version matching")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

include(DiscoverNeuron)
include(VersionGate)

# ---------------------------------------------------------------------------
# M1: smoke tests only. Integration groups arrive in M2+.
# ---------------------------------------------------------------------------

# Placeholder for M2+: custom target that will depend on nrnivmodl jobs.
add_custom_target(foreign COMMENT "Foreign mechanism builds (none registered yet in M1)")

# Isolate tests from a developer’s ambient PYTHONPATH (build-tree NEURON).
# Keep the foreign Python’s directory first on PATH so nrniv/nrnivmodl resolve
# to the same environment used for discovery.
get_filename_component(_nrn_foreign_py_bindir "${NRN_FOREIGN_PYTHON}" DIRECTORY)
set(_nrn_foreign_test_env "PYTHONPATH=" "PATH=${_nrn_foreign_py_bindir}:$ENV{PATH}")

if(NRN_FOREIGN_NRNIV STREQUAL "")
  message(WARNING "nrniv not found on PATH for foreign Python; nrniv smoke test skipped")
else()
  add_test(
    NAME foreign::smoke_nrniv
    COMMAND "${NRN_FOREIGN_NRNIV}" -nogui -nopython -nobanner -c "quit()")
  set_tests_properties(
    foreign::smoke_nrniv PROPERTIES LABELS "foreign;serial;smoke" TIMEOUT 60 ENVIRONMENT
                                    "${_nrn_foreign_test_env}")
endif()

add_test(
  NAME foreign::smoke_import
  COMMAND "${NRN_FOREIGN_PYTHON}" -c "import neuron; print(neuron.__version__)")
set_tests_properties(
  foreign::smoke_import PROPERTIES LABELS "foreign;serial;smoke" TIMEOUT 60 ENVIRONMENT
                                   "${_nrn_foreign_test_env}")

# neuron.test() is the classic wheel smoke; slower than import-only.
option(NRN_FOREIGN_SMOKE_NEURON_TEST "Run neuron.test() as a foreign smoke test" ON)
if(NRN_FOREIGN_SMOKE_NEURON_TEST)
  add_test(
    NAME foreign::smoke_neuron_test
    COMMAND "${NRN_FOREIGN_PYTHON}" -c
            "import neuron; raise SystemExit(0 if neuron.test() else 1)")
  set_tests_properties(
    foreign::smoke_neuron_test PROPERTIES LABELS "foreign;serial;smoke" TIMEOUT 600
                                          ENVIRONMENT "${_nrn_foreign_test_env}")
endif()

message(STATUS "Foreign ctest source root : ${NRN_FOREIGN_SOURCE_ROOT}")
message(STATUS "Configure complete. Build target 'foreign' (no-op in M1), then ctest.")
