# Standalone project: run NEURON integration tests against a foreign (wheel) install. See README.md.
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

if(NOT DEFINED NRN_FOREIGN_PATH_PREFIX OR NRN_FOREIGN_PATH_PREFIX STREQUAL "")
  get_filename_component(NRN_FOREIGN_PATH_PREFIX "${NRN_FOREIGN_PYTHON}" DIRECTORY)
endif()
# Prefer prefix site path when set; otherwise empty PYTHONPATH (wheel/site-packages).
if(DEFINED NRN_FOREIGN_SITE_PYTHONPATH AND NOT NRN_FOREIGN_SITE_PYTHONPATH STREQUAL "")
  set(_nrn_foreign_smoke_pythonpath "${NRN_FOREIGN_SITE_PYTHONPATH}")
else()
  set(_nrn_foreign_smoke_pythonpath "")
endif()
set(_nrn_foreign_test_env "PYTHONPATH=${_nrn_foreign_smoke_pythonpath}"
                          "PATH=${NRN_FOREIGN_PATH_PREFIX}${NRN_FOREIGN_ENV_SEP}$ENV{PATH}")

# ---------------------------------------------------------------------------
# Smoke tests (M1)
# ---------------------------------------------------------------------------
if(NRN_FOREIGN_NRNIV STREQUAL "")
  message(WARNING "nrniv not found on PATH for foreign Python; nrniv smoke test skipped")
else()
  add_test(NAME foreign::smoke_nrniv COMMAND "${NRN_FOREIGN_NRNIV}" -nogui -nopython -nobanner -c
                                             "quit()")
  set_tests_properties(foreign::smoke_nrniv PROPERTIES LABELS "foreign;serial;smoke" TIMEOUT 60
                                                       ENVIRONMENT "${_nrn_foreign_test_env}")
endif()

add_test(NAME foreign::smoke_import COMMAND "${NRN_FOREIGN_PYTHON}" -c
                                            "import neuron; print(neuron.__version__)")
set_tests_properties(foreign::smoke_import PROPERTIES LABELS "foreign;serial;smoke" TIMEOUT 60
                                                      ENVIRONMENT "${_nrn_foreign_test_env}")

option(NRN_FOREIGN_SMOKE_NEURON_TEST "Run neuron.test() as a foreign smoke test" ON)
if(NRN_FOREIGN_SMOKE_NEURON_TEST)
  add_test(NAME foreign::smoke_neuron_test
           COMMAND "${NRN_FOREIGN_PYTHON}" -c
                   "import neuron; raise SystemExit(0 if neuron.test() else 1)")
  set_tests_properties(
    foreign::smoke_neuron_test PROPERTIES LABELS "foreign;serial;smoke" TIMEOUT 600 ENVIRONMENT
                                          "${_nrn_foreign_test_env}")
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
  message(WARNING "pytest is not installed for ${NRN_FOREIGN_PYTHON}. "
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

# Default install check: build mechanisms, run serial ctest, print how to re-run ctest. Full
# filtering stays with plain ctest --test-dir <this binary dir> ...
set(NRN_FOREIGN_TEST_INSTALL_LABELS
    "serial"
    CACHE STRING "CTest -L expression used by the test-install target")
set(NRN_FOREIGN_TEST_INSTALL_JOBS
    "4"
    CACHE STRING "ctest -j parallelism for the test-install target")
add_custom_target(
  test-install
  COMMAND
    ${CMAKE_COMMAND} "-DNRN_FOREIGN_CTEST_COMMAND=${CMAKE_CTEST_COMMAND}"
    "-DNRN_FOREIGN_BINARY_DIR=${CMAKE_BINARY_DIR}"
    "-DNRN_FOREIGN_TEST_INSTALL_LABELS=${NRN_FOREIGN_TEST_INSTALL_LABELS}"
    "-DNRN_FOREIGN_TEST_INSTALL_JOBS=${NRN_FOREIGN_TEST_INSTALL_JOBS}" -P
    "${CMAKE_CURRENT_SOURCE_DIR}/RunTestInstall.cmake"
  DEPENDS foreign
  USES_TERMINAL
  COMMENT
    "Foreign install check: ctest -L ${NRN_FOREIGN_TEST_INSTALL_LABELS} (see target foreign + ctest)"
)

# Configure-time summary (how many tests landed in each bucket)
set(_n_serial 0)
set(_n_mpi 0)
set(_n_cn 0)
set(_n_all 0)
get_property(
  _all_tests
  DIRECTORY
  PROPERTY TESTS)
foreach(_t ${_all_tests})
  math(EXPR _n_all "${_n_all}+1")
  get_test_property("${_t}" LABELS _labs)
  if(_labs)
    if("serial" IN_LIST _labs)
      math(EXPR _n_serial "${_n_serial}+1")
    endif()
    if("mpi" IN_LIST _labs)
      math(EXPR _n_mpi "${_n_mpi}+1")
    endif()
    if("coreneuron" IN_LIST _labs)
      math(EXPR _n_cn "${_n_cn}+1")
    endif()
  endif()
endforeach()

message(STATUS "Foreign ctest source root : ${NRN_FOREIGN_SOURCE_ROOT}")
message(STATUS "Foreign tests registered  : ${_n_all} total"
               " (serial≈${_n_serial}, mpi≈${_n_mpi}, coreneuron≈${_n_cn})")
message(STATUS "Inventory                 : ${CMAKE_CURRENT_SOURCE_DIR}/INVENTORY.md")
message(STATUS "Configure complete. cmake --build <build> --target test-install")
message(STATUS "  (or: --target foreign && ctest --test-dir <build> -L serial)")
