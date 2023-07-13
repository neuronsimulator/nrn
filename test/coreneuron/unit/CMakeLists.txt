# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================

include(TestHelpers)

# Add compiler flags that should apply to all CoreNEURON targets, but which should not leak into
# other included projects.
add_compile_definitions(${CORENRN_COMPILE_DEFS})
add_compile_options(${CORENRN_EXTRA_CXX_FLAGS})
add_link_options(${CORENRN_EXTRA_LINK_FLAGS})

set(CMAKE_BUILD_RPATH ${CMAKE_BINARY_DIR}/bin/${CMAKE_HOST_SYSTEM_PROCESSOR})
if(CORENRN_ENABLE_UNIT_TESTS)
  include_directories(${PROJECT_SOURCE_DIR}/src)
  add_library(coreneuron-unit-test INTERFACE)
  target_include_directories(coreneuron-unit-test SYSTEM INTERFACE ${CMAKE_BINARY_DIR}/include)
  target_link_libraries(coreneuron-unit-test INTERFACE coreneuron-all)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cmdline_interface)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/interleave_info)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/alignment)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/queueing)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/solver)
  # lfp test uses nrnmpi_* wrappers but does not load the dynamic MPI library TODO: re-enable after
  # NEURON and CoreNEURON dynamic MPI are merged
  if(NOT NRN_ENABLE_MPI_DYNAMIC)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lfp)
  endif()
endif()
