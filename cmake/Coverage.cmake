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
#
#   NRN_COVERAGE_FILES speeds the workflow tremendously, when iteratively
#   working on a single or a few files.
#
#   Two targets are created: cover_begin and cover_html.
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
  if(LCOV STREQUAL "LCOV-NOTFOUND")
    message(ERROR "lcov is required with NRN_ENABLE_COVERAGE=ON and it was not found.")
  endif()
  string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
  if(NOT BUILD_TYPE_UPPER STREQUAL "DEBUG")
    message(WARNING "Using CMAKE_BUILD_TYPE=Debug is recommended with NRN_ENABLE_COVERAGE")
  endif()
  set(NRN_COVERAGE_FLAGS_UNQUOTED --coverage -fno-inline)
  string(JOIN " " NRN_COVERAGE_FLAGS ${NRN_COVERAGE_FLAGS_UNQUOTED})
  set(NRN_COVERAGE_LINK_FLAGS --coverage)

  if(NRN_COVERAGE_FILES)
    # ~~~
    # cannot figure out how to set specific file flags here. So they are
    # are set in src/nrniv/CMakeLists.txt
    # Surprisingly to me, this works for src/nrnpython files even though
    # they are compiled and linked in src/nrnpython/CMakeLists.txt.
    # I.e. successful with
    # -DNRN_COVERAGE_FILES="src/nrniv/partrans.cpp;src/nmodl/parsact.cpp;src/nrnpython/nrnpy_hoc.cpp"
    # ~~~
    if(NRN_ADDED_COVERAGE_FLAGS)
      message(
        FATAL_ERROR
          "NRN_ENABLE_COVERAGE=ON and there are NRN_COVERAGE_FILES, but full coverage of all files is in effect from a previous setting. Please empty the build folder and start over."
      )
    endif()
  else()
    # cannot be turned off without starting from scratch.
    set(NRN_ADDED_COVERAGE_FLAGS
        "${NRN_COVERAGE_FLAGS}"
        CACHE INTERNAL "Remind that this is always in effect from now on" FORCE)
    list(APPEND NRN_COMPILE_FLAGS ${NRN_COVERAGE_FLAGS_UNQUOTED})
    list(APPEND CORENRN_EXTRA_CXX_FLAGS ${NRN_COVERAGE_FLAGS_UNQUOTED})
    list(APPEND CORENRN_EXTRA_MECH_CXX_FLAGS ${NRN_COVERAGE_FLAGS_UNQUOTED})
  endif()
  list(APPEND NRN_LINK_FLAGS ${NRN_COVERAGE_LINK_FLAGS})
  list(APPEND CORENRN_EXTRA_LINK_FLAGS ${NRN_COVERAGE_LINK_FLAGS})
  list(APPEND NRN_COMPILE_DEFS NRN_COVERAGE_ENABLED)
else()
  unset(NRN_COVERAGE_FLAGS)
  unset(NRN_COVERAGE_FILES CACHE)
  if(NRN_ADDED_COVERAGE_FLAGS)
    message(
      FATAL_ERROR
        "NRN_ENABLE_COVERAGE=OFF but full coverage of all files is in effect from a previous setting. Please empty the build folder and start over."
    )
  endif()
endif()

if(NRN_ENABLE_COVERAGE)
  set(cover_clean_command find "${PROJECT_BINARY_DIR}" "-name" "*.gcda" "-type" "f" "-delete")
  set(cover_baseline_command
      "${LCOV}" "--capture" "--initial" "--no-external" "--directory" "${PROJECT_SOURCE_DIR}"
      "--directory" "${PROJECT_BINARY_DIR}" "--output-file" "coverage-base.info")
  set(cover_collect_command
      "${LCOV}" "--capture" "--no-external" "--directory" "${PROJECT_SOURCE_DIR}" "--directory"
      "${PROJECT_BINARY_DIR}" "--output-file" "coverage-run.info")
  set(cover_combine_command "${LCOV}" "--add-tracefile" "coverage-base.info" "--add-tracefile"
                            "coverage-run.info" "--output-file" "coverage-combined.info")
  set(cover_html_command genhtml "coverage-combined.info" "--output-directory" html)
  add_custom_target(
    cover_clean
    COMMAND ${cover_clean_command}
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
  add_custom_target(
    cover_baseline
    COMMAND ${cover_baseline_command}
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
  add_custom_target(
    cover_begin
    COMMAND ${cover_clean_command}
    COMMAND ${cover_baseline_command}
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
  add_custom_target(
    cover_collect
    COMMAND ${cover_collect_command}
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
  add_custom_target(
    cover_combine
    COMMAND ${cover_combine_command}
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
  add_custom_target(
    cover_html
    COMMAND ${cover_collect_command}
    COMMAND ${cover_combine_command}
    COMMAND ${cover_html_command}
    COMMAND echo "View in browser at file://${PROJECT_BINARY_DIR}/html/index.html"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
endif()
