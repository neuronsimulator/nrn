# Configure NeuronTestHelper.cmake for a foreign (wheel) NEURON install. Call after DiscoverNeuron /
# VersionGate, with NRN_FOREIGN_SOURCE_ROOT set.

set(NRN_FOREIGN_MODE ON)
set(NRN_TEST_SOURCE_ROOT "${NRN_FOREIGN_SOURCE_ROOT}")
set(NRN_TEST_BINARY_ROOT "${CMAKE_BINARY_DIR}")
set(NRN_NRNIVMODL "${NRN_FOREIGN_NRNIVMODL}")
# No in-tree libnrniv / CoreNEURON targets to depend on.
set(NRN_NRNIVMODL_DEPENDS "")

# Map discovered features onto the variables NeuronTestHelper expects.
set(NRN_ENABLE_PYTHON ${NRN_FOREIGN_FEATURE_NRN_ENABLE_PYTHON})
set(NRN_ENABLE_MPI ${NRN_FOREIGN_FEATURE_NRN_ENABLE_MPI})
set(NRN_ENABLE_MPI_DYNAMIC ${NRN_FOREIGN_FEATURE_NRN_ENABLE_MPI_DYNAMIC})
set(NRN_ENABLE_CORENEURON ${NRN_FOREIGN_FEATURE_NRN_ENABLE_CORENEURON})
set(NRN_ENABLE_RX3D ${NRN_FOREIGN_FEATURE_NRN_ENABLE_RX3D})
set(NRN_ENABLE_THREADS ${NRN_FOREIGN_FEATURE_NRN_ENABLE_THREADS})
set(NRN_ENABLE_MUSIC ${NRN_FOREIGN_FEATURE_NRN_ENABLE_MUSIC})
set(CORENRN_ENABLE_GPU ${NRN_FOREIGN_FEATURE_CORENRN_ENABLE_GPU})
set(CORENRN_ENABLE_SHARED ${NRN_FOREIGN_FEATURE_CORENRN_ENABLE_SHARED})
# Wheels with CoreNEURON use mod compatibility; without CN leave off.
if(NRN_ENABLE_CORENEURON)
  set(NRN_ENABLE_MOD_COMPATIBILITY ON)
else()
  set(NRN_ENABLE_MOD_COMPATIBILITY OFF)
endif()

if(NRN_FOREIGN_NRNIVMODL STREQUAL "")
  message(FATAL_ERROR "Foreign nrnivmodl not found; cannot register mechanism-based tests")
endif()

get_filename_component(_nrn_foreign_py_bindir "${NRN_FOREIGN_PYTHON}" DIRECTORY)
if(NOT DEFINED NRN_FOREIGN_PATH_PREFIX OR NRN_FOREIGN_PATH_PREFIX STREQUAL "")
  set(NRN_FOREIGN_PATH_PREFIX "${_nrn_foreign_py_bindir}")
endif()

# Environment for nrnivmodl + tests: venv/prefix tools first. PYTHONPATH is set (not empty) so
# ambient build-tree paths are replaced, while still exposing in-tree helpers such as
# test/rxd/testutils.py. Site-packages from the wheel remain on sys.path via the usual site
# mechanism.
set(NRN_RUN_FROM_BUILD_DIR_ENV "PATH=${NRN_FOREIGN_PATH_PREFIX}:$ENV{PATH}"
                               "PYTHONPATH=${NRN_FOREIGN_SOURCE_ROOT}/test/rxd")

# Prefer the foreign interpreter for any test that runs Python.
set(NRN_DEFAULT_PYTHON_EXECUTABLE "${NRN_FOREIGN_PYTHON}")

# MPI launcher (absolute path preferred so tests work if mpiexec is not first on PATH).
set(MPIEXEC_NAME "${NRN_FOREIGN_MPIEXEC}")
set(MPIEXEC_NUMPROC_FLAG
    "-n"
    CACHE STRING "mpiexec flag for process count")
set(MPIEXEC_PREFLAGS
    ""
    CACHE STRING "mpiexec flags before the executable")
set(MPIEXEC_POSTFLAGS
    ""
    CACHE STRING "mpiexec flags after the executable")
set(MPIEXEC_OVERSUBSCRIBE "")
if(NRN_ENABLE_MPI AND NOT MPIEXEC_NAME STREQUAL "")
  execute_process(
    COMMAND ${MPIEXEC_NAME} --oversubscribe --version
    RESULT_VARIABLE _over_rc
    OUTPUT_QUIET ERROR_QUIET)
  if(_over_rc EQUAL 0)
    set(MPIEXEC_OVERSUBSCRIBE "--oversubscribe")
  endif()
  get_filename_component(_nrn_foreign_mpiexec_dir "${MPIEXEC_NAME}" DIRECTORY)
  # Ensure launcher directory is on PATH for nested tools.
  set(NRN_RUN_FROM_BUILD_DIR_ENV
      "PATH=${NRN_FOREIGN_PATH_PREFIX}:${_nrn_foreign_mpiexec_dir}:$ENV{PATH}"
      "PYTHONPATH=${NRN_FOREIGN_SOURCE_ROOT}/test/rxd")
  message(STATUS "Foreign mpiexec           : ${MPIEXEC_NAME}")
  message(STATUS "Foreign mpiexec oversub   : ${MPIEXEC_OVERSUBSCRIBE}")
endif()

# coding-conventions is optional for foreign mode (fallback copy in NeuronTestHelper).
if(EXISTS "${NRN_FOREIGN_SOURCE_ROOT}/external/coding-conventions/cpp/cmake/build-time-copy.cmake")
  set(CODING_CONV_CMAKE "${NRN_FOREIGN_SOURCE_ROOT}/external/coding-conventions/cpp/cmake")
endif()

include("${NRN_FOREIGN_SOURCE_ROOT}/cmake/NeuronTestHelper.cmake")

# Collect nrnivmodl custom targets so the top-level `foreign` target can depend on them.
set(NRN_FOREIGN_NRNIVMODL_TARGETS
    ""
    CACHE INTERNAL "nrnivmodl targets registered for foreign ctest")

function(nrn_foreign_track_prep_target target_name)
  if(target_name AND TARGET "${target_name}")
    set(_all "${NRN_FOREIGN_NRNIVMODL_TARGETS}")
    list(APPEND _all "${target_name}")
    list(REMOVE_DUPLICATES _all)
    set(NRN_FOREIGN_NRNIVMODL_TARGETS
        "${_all}"
        CACHE INTERNAL "nrnivmodl targets registered for foreign ctest" FORCE)
  endif()
endfunction()

function(nrn_foreign_note_nrnivmodl_group group_name)
  set(prefix NRN_TEST_GROUP_${group_name})
  nrn_foreign_track_prep_target("${${prefix}_NRNIVMODL_TARGET_NAME}")
endfunction()

# After nrn_add_test(GROUP g NAME n ...), hang copy-scripts and labels off `foreign`. Extra ARGN
# tokens become additional CTest labels. Default label "serial" is added unless ARGN includes "mpi"
# or "noserial".
function(nrn_foreign_finalize_test group_name test_name)
  set(copy_tgt "copy-scripts-${group_name}-${test_name}")
  nrn_foreign_track_prep_target("${copy_tgt}")
  set(full_name "${group_name}::${test_name}")
  # LABELS must be one property value (semicolon-separated). Bare tokens after LABELS are parsed as
  # additional PROPERTY names.
  set(_labels "foreign")
  set(_add_serial ON)
  foreach(_l ${ARGN})
    if(_l STREQUAL "mpi" OR _l STREQUAL "noserial")
      set(_add_serial OFF)
    endif()
  endforeach()
  if(_add_serial)
    string(APPEND _labels ";serial")
  endif()
  foreach(_l ${ARGN})
    if(NOT _l STREQUAL "noserial")
      string(APPEND _labels ";${_l}")
    endif()
  endforeach()
  set_tests_properties("${full_name}" PROPERTIES LABELS "${_labels}")
endfunction()
