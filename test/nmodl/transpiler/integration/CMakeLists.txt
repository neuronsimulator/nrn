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
  if(NRN_ENABLE_BACKTRACE)
    add_test(NAME ${modfile_name}_with_blame COMMAND $<TARGET_FILE:nmodl> ${modfile} blame --line 2)
  endif()
  if(WIN32)
    set(NMODL_TEST_PYTHONPATH "${PROJECT_BINARY_DIR}/lib;$ENV{PYTHONPATH}")
  else()
    set(NMODL_TEST_PYTHONPATH "${PROJECT_BINARY_DIR}/lib:$ENV{PYTHONPATH}")
  endif()
  if(NRN_ENABLE_BACKTRACE)
    set(tests ${modfile_name} ${modfile_name}_with_blame)
  else()
    set(tests ${modfile_name})
  endif()
  foreach(test IN LISTS tests)
    set_tests_properties(
      ${test}
      PROPERTIES
        ENVIRONMENT
        "PYTHONPATH=${NMODL_TEST_PYTHONPATH};NMODL_PYLIB=${PYTHON_LIBRARY};NMODLHOME=${CMAKE_BINARY_DIR}"
    )
    cpp_cc_configure_sanitizers(TEST ${test})
  endforeach()
endforeach()
