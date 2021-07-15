# =============================================================================
# translation of mod files
# =============================================================================
file(GLOB modfiles "${NMODL_PROJECT_SOURCE_DIR}/test/integration/mod/*.mod")
foreach(modfile ${modfiles})
  get_filename_component(modfile_name "${modfile}" NAME)
  add_test(NAME ${modfile_name} COMMAND ${PROJECT_BINARY_DIR}/bin/nmodl ${modfile})
endforeach()
