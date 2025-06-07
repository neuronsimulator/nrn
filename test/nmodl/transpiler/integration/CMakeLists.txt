# =============================================================================
# Add extra compile flags to NMODL test sources
# =============================================================================
add_compile_options(${NMODL_EXTRA_CXX_FLAGS})
add_link_options(${NMODL_EXTRA_CXX_FLAGS})

# =============================================================================
# translation of mod files
# =============================================================================
file(GLOB modfiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mod/*.mod")
foreach(modfile ${modfiles})
  get_filename_component(modfile_name "${modfile}" NAME)
  set(test_name "nmodl::integration::${modfile_name}")
  add_test(NAME ${test_name} COMMAND ${CMAKE_BINARY_DIR}/bin/nmodl ${modfile})
  if(WIN32)
    set(NMODL_TEST_PYTHONPATH "${PROJECT_BINARY_DIR}/lib;$ENV{PYTHONPATH}")
  else()
    set(NMODL_TEST_PYTHONPATH "${PROJECT_BINARY_DIR}/lib:$ENV{PYTHONPATH}")
  endif()
  set_tests_properties(
    ${test_name}
    PROPERTIES
      ENVIRONMENT
      "PYTHONPATH=${NMODL_TEST_PYTHONPATH};NMODL_PYLIB=${PYTHON_LIBRARY};NMODLHOME=${CMAKE_BINARY_DIR}"
  )
  cpp_cc_configure_sanitizers(TEST ${test_name})
endforeach()
