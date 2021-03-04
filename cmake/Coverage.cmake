# =========================================================
# Configure support for Code Coverage
# =========================================================
# ~~~
# Verify lcov is available.
# NRN_ENABLE_COVERAGE=ON
#   Compile appropriate files (all files or those specified in semicolon
#   separated list of files in NRN_COVERAGE_FILES (relative to
#   PROJECT_SOURCE_DIR) with code coverage enabled.
#   NRN_COVERAGE_FILES="" (default) means all files.
#   Two targets will be created: cover_begin and cover_html.
#
#   cover_begin erases all the *.gcda coverage files and
#   creates a baseline report (coverage-base.info)
#
#   cover_html creates a report with all coverage so far (coverage-run.info),
#   combines that with coverage_base_info to create coverage-combined.info,
#   prints a summary of the coverage-combined.info and creates an html report
#   in the html folder
#
#   All created files (folders) are relative to PROJECT_BINARY_DIR.   
# ~~~

if(NRN_ENABLE_COVERAGE)
  find_program(LCOV lcov)
  if (LCOV-NOTFOUND)
    message(ERROR "lcov is not installed.")
  endif()
  set(NRN_COVERAGE_FLAGS "--coverage -O0 -fno-inline -g")
  if (NRN_COVERAGE_FILES)

  else()
    set(CMAKE_C_FLAGS="${NRN_COVERAGE_FLAGS}")
    set(CMAKE_CXX_FLAGS="${NRN_COVERAGE_FLAGS}")
  endif()
else()
  unset(NRN_COVERAGE_FLAGS)
endif()

if(NRN_ENABLE_COVERAGE)

  add_custom_target(
    cover_begin
    COMMAND find "${PROJECT_BINARY_DIR}" "-name" "*.gcda" "-type" "f" "-delete"
    COMMAND "${LCOV}"  "--capture" "--initial" "--no-external" "--directory" "${PROJECT_SOURCE_DIR}" "--directory" "${PROJECT_BINARY_DIR}" "--output-file" "coverage-base.info"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )

  add_custom_target(
    cover_html
    COMMAND ${LCOV} "--capture" "--no-external" "--directory" "${PROJECT_SOURCE_DIR}" "--directory" ${PROJECT_BINARY_DIR} "--output-file" "coverage-run.info"
    COMMAND "${LCOV}" "--add-tracefile" "coverage-base.info" "--add-tracefile" "coverage-run.info" "--output-file" "coverage-combined.info"
    COMMAND genhtml "coverage-combined.info" "--output-directory" html
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )
endif()
