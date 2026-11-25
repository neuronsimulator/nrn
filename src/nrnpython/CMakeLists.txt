# Add directory-level default compiler flags -- these should be added to all NEURON targets, but not
# targets from included projects like CoreNEURON and NMODL
add_compile_options(${NRN_COMPILE_FLAGS})
add_compile_definitions(${NRN_COMPILE_DEFS})
add_link_options(${NRN_LINK_FLAGS})

include(${PROJECT_SOURCE_DIR}/cmake/PythonCompileHelper.cmake)

# Some modules should be placed in the `neuron` Python directory (as they are directly importable),
# while others should be placed in `neuron/.data/lib` (as they are not directly importable).
if(NOT SKBUILD)
  file(RELATIVE_PATH rel_path "${CMAKE_INSTALL_PREFIX}/${NRN_INSTALL_PYTHON_PREFIX}"
       "${CMAKE_INSTALL_PREFIX}/${NRN_INSTALL_DATA_PREFIX}/lib")
  set(libraries_rpath "/${rel_path}")
else()
  set(libraries_rpath "/.data/lib/")
endif()

# =============================================================================
# rxdmath libraries (always build)
# =============================================================================
add_library(rxdmath SHARED ${PROJECT_SOURCE_DIR}/src/rxd/rxdmath.cpp)
install(TARGETS rxdmath DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})

# =============================================================================
# modl_reg stub for the Python interface (see modlreg_stub.c)
# =============================================================================
add_library(nrn_modlreg_stub SHARED ${CMAKE_CURRENT_SOURCE_DIR}/modlreg_stub.c)
install(TARGETS nrn_modlreg_stub DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})

configure_file(_config_params.py.in "${PROJECT_BINARY_DIR}/lib/python/neuron/_config_params.py"
               @ONLY)

install(FILES "${PROJECT_BINARY_DIR}/lib/python/neuron/_config_params.py"
        DESTINATION ${NRN_INSTALL_PYTHON_PREFIX})
