# Configure NeuronTestHelper.cmake for a foreign (wheel) NEURON install.
# Call after DiscoverNeuron / VersionGate, with NRN_FOREIGN_SOURCE_ROOT set.

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

# Environment for nrnivmodl + tests: venv tools first.
# PYTHONPATH is set (not empty) so ambient build-tree paths are replaced, while
# still exposing in-tree helpers such as test/rxd/testutils.py. Site-packages
# from the wheel remain on sys.path via the usual site mechanism.
set(NRN_RUN_FROM_BUILD_DIR_ENV
    "PATH=${_nrn_foreign_py_bindir}:$ENV{PATH}"
    "PYTHONPATH=${NRN_FOREIGN_SOURCE_ROOT}/test/rxd")

# Prefer the foreign interpreter for any test that runs Python.
set(NRN_DEFAULT_PYTHON_EXECUTABLE "${NRN_FOREIGN_PYTHON}")

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

# After nrn_add_test(GROUP g NAME n ...), hang copy-scripts and labels off `foreign`.
# Extra ARGN tokens become additional CTest labels.
function(nrn_foreign_finalize_test group_name test_name)
  set(copy_tgt "copy-scripts-${group_name}-${test_name}")
  nrn_foreign_track_prep_target("${copy_tgt}")
  set(full_name "${group_name}::${test_name}")
  # LABELS must be one property value (semicolon-separated). Bare tokens after
  # LABELS are parsed as additional PROPERTY names.
  set(_labels "foreign;serial")
  foreach(_l ${ARGN})
    string(APPEND _labels ";${_l}")
  endforeach()
  set_tests_properties("${full_name}" PROPERTIES LABELS "${_labels}")
endfunction()
