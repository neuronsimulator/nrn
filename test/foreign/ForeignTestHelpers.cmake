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

# Environment for nrnivmodl + tests: venv/prefix tools first. PYTHONPATH: optional prefix lib/python
# (classic install) + in-tree test/rxd helpers. Wheels keep using site-packages via the
# interpreter's site mechanism when the prefix entry is empty.
set(_nrn_foreign_test_pythonpath "${NRN_FOREIGN_SOURCE_ROOT}/test/rxd")
if(DEFINED NRN_FOREIGN_SITE_PYTHONPATH AND NOT NRN_FOREIGN_SITE_PYTHONPATH STREQUAL "")
  set(_nrn_foreign_test_pythonpath
      "${NRN_FOREIGN_SITE_PYTHONPATH}${NRN_FOREIGN_ENV_SEP}${_nrn_foreign_test_pythonpath}")
endif()

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
  message(STATUS "Foreign mpiexec           : ${MPIEXEC_NAME}")
  message(STATUS "Foreign mpiexec oversub   : ${MPIEXEC_OVERSUBSCRIBE}")
endif()

# coding-conventions is optional for foreign mode (fallback copy in NeuronTestHelper).
if(EXISTS "${NRN_FOREIGN_SOURCE_ROOT}/external/coding-conventions/cpp/cmake/build-time-copy.cmake")
  set(CODING_CONV_CMAKE "${NRN_FOREIGN_SOURCE_ROOT}/external/coding-conventions/cpp/cmake")
endif()

# Overlay of test/foreign onto another checkout still needs the 3829 helper (msvc-portability's
# helper has no NRN_FOREIGN_MODE). Prefer a copy next to this file; otherwise the parent tree's
# cmake/NeuronTestHelper.cmake.
set(NRN_FOREIGN_VCVARS "")
if(WIN32)
  foreach(
    _nrn_vcvars IN
    ITEMS
      "$ENV{VSINSTALLDIR}VC/Auxiliary/Build/vcvars64.bat"
      "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat"
      "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat"
      "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Auxiliary/Build/vcvars64.bat")
    if(EXISTS "${_nrn_vcvars}")
      set(NRN_FOREIGN_VCVARS "${_nrn_vcvars}")
      break()
    endif()
  endforeach()
  if(NRN_FOREIGN_VCVARS)
    message(STATUS "Foreign vcvars64          : ${NRN_FOREIGN_VCVARS}")
  endif()
endif()
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/NeuronTestHelper.cmake")
  include("${CMAKE_CURRENT_LIST_DIR}/NeuronTestHelper.cmake")
else()
  include("${NRN_FOREIGN_SOURCE_ROOT}/cmake/NeuronTestHelper.cmake")
endif()

nrn_foreign_cmake_env_path(_nrn_run_path "${NRN_FOREIGN_PATH_PREFIX}")
if(DEFINED _nrn_foreign_mpiexec_dir AND NOT _nrn_foreign_mpiexec_dir STREQUAL "")
  nrn_foreign_cmake_env_path(_nrn_run_path "${NRN_FOREIGN_PATH_PREFIX}"
                             "${_nrn_foreign_mpiexec_dir}")
endif()
# CMake lists split on ';'. Prefix + test/rxd is two Windows PYTHONPATH entries; without escaping,
# cmake -E env treats the second as the program ("no such file or directory"). Same trick as
# nrn_foreign_cmake_env_path.
string(REPLACE ";" "\\;" _nrn_foreign_test_pythonpath_esc "${_nrn_foreign_test_pythonpath}")
set(NRN_RUN_FROM_BUILD_DIR_ENV "${_nrn_run_path}" "PYTHONPATH=${_nrn_foreign_test_pythonpath_esc}")

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
