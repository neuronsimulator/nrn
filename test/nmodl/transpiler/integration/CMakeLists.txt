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
  add_test(NAME ${modfile_name} COMMAND $<TARGET_FILE:nmodl> ${modfile})
  if(WIN32)
    set(NMODL_TEST_PYTHONPATH "${PROJECT_BINARY_DIR}/lib;$ENV{PYTHONPATH}")
  else()
    set(NMODL_TEST_PYTHONPATH "${PROJECT_BINARY_DIR}/lib:$ENV{PYTHONPATH}")
  endif()
  set_tests_properties(
    ${modfile_name}
    PROPERTIES
      ENVIRONMENT
      "PYTHONPATH=${NMODL_TEST_PYTHONPATH};NMODL_PYLIB=${PYTHON_LIBRARY};NMODLHOME=${CMAKE_BINARY_DIR}"
  )
  cpp_cc_configure_sanitizers(TEST ${modfile_name})
endforeach()
