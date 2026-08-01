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
include(ForeignTestHelpers)

get_filename_component(_nrn_foreign_py_bindir "${NRN_FOREIGN_PYTHON}" DIRECTORY)
set(_nrn_foreign_test_env "PYTHONPATH=" "PATH=${_nrn_foreign_py_bindir}:$ENV{PATH}")

# ---------------------------------------------------------------------------
# Smoke tests (M1)
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# Serial portable integration suite (M3)
# ---------------------------------------------------------------------------
set(_nrn_foreign_have_pytest OFF)
execute_process(
  COMMAND "${NRN_FOREIGN_PYTHON}" -c "import pytest"
  RESULT_VARIABLE _pytest_rc
  OUTPUT_QUIET ERROR_QUIET)
if(_pytest_rc EQUAL 0)
  set(_nrn_foreign_have_pytest ON)
else()
  message(
    WARNING
      "pytest is not installed for ${NRN_FOREIGN_PYTHON}. "
      "Install with: \"${NRN_FOREIGN_PYTHON}\" -m pip install pytest\n"
      "Python pytest-based groups will be skipped.")
endif()

include(SerialPortableTests.cmake)
include(MpiCoreNeuronTests.cmake)

# Target that builds all foreign nrnivmodl (special) jobs and test prep.
add_custom_target(foreign COMMENT "Build mechanisms for foreign ctest groups")
if(NRN_FOREIGN_NRNIVMODL_TARGETS)
  add_dependencies(foreign ${NRN_FOREIGN_NRNIVMODL_TARGETS})
  list(LENGTH NRN_FOREIGN_NRNIVMODL_TARGETS _nprep)
  message(STATUS "Foreign prep targets (${_nprep})")
else()
  message(STATUS "Foreign prep targets    : (none)")
endif()

message(STATUS "Foreign ctest source root : ${NRN_FOREIGN_SOURCE_ROOT}")
message(STATUS "Configure complete. cmake --build <build> --target foreign && ctest -L serial")
