# =============================================================================
# Given list of file names, find their path in project source tree
# =============================================================================
macro(nrn_find_project_files list_name)
  foreach(name ${ARGN})
    execute_process(
      COMMAND find ${PROJECT_SOURCE_DIR}/src -name ${name}
      RESULTS_VARIABLE result
      OUTPUT_VARIABLE filepath
    )
    if ( (NOT result EQUAL 0) OR filepath STREQUAL "")
      message(FATAL_ERROR " ${name} not found in ${PROJECT_SOURCE_DIR}/src")
    else()
      string(REGEX REPLACE "\n$" "" filepath "${filepath}")
      list(APPEND ${list_name} ${filepath})
    endif()
  endforeach(name)
endmacro()