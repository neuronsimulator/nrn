# Function for adding a test _at configure-time_ (because CTest does not work otherwise) using
# Pytest.
function(pytest_add_test)
  set(options REPLACE_MISSING_IMPORTS)
  set(oneValueArgs TARGET PYTHON_EXECUTABLE SOURCE DESTINATION WORKING_DIRECTORY)
  # the list of modules to remove from the AST (since they are most likely imported)
  set(multiValueArgs MODULES PYTEST_ARGS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT ARG_SOURCE)
    message(FATAL_ERROR "You must provide the `SOURCE` parameter to `pytest_add_test`")
  endif()
  if(NOT ARG_DESTINATION)
    # if no destination, assume it is the current file
    set(destination "${ARG_SOURCE}")
  else()
    set(destination "${ARG_DESTINATION}")
  endif()
  if(NOT ARG_WORKING_DIRECTORY)
    # wherever is the current source file
    get_filename_component(working_directory "${ARG_SOURCE}" DIRECTORY)
  else()
    # check it's actually a directory
    if(NOT IS_DIRECTORY "${ARG_WORKING_DIRECTORY}")
      message(
        FATAL_ERROR "The `WORKING_DIRECTORY` parameter to `pytest_add_test` must be a directory")
    endif()
    set(working_directory "${ARG_WORKING_DIRECTORY}")
  endif()
  if(NOT ARG_REPLACE_MISSING_IMPORTS)
    set(replace_missing_imports OFF)
  else()
    if(NOT ARG_MODULES)
      message(
        FATAL_ERROR
          "With `REPLACE_MISSING_IMPORTS`, you must provide at least one module to the `MODULES` parameter to `pyest_add_test`"
      )
    endif()
    set(replace_missing_imports ON)
  endif()
  if(NOT ARG_PYTHON_EXECUTABLE)
    # inform the user we are guessing
    message(
      WARNING "No value for `PYTHON_EXECUTABLE` in `pytest_add_test`; using `find_package` naively")
    find_package(Python 3.9 REQUIRED)
    set(pyexe "${PYTHON_EXECUTABLE}")
  else()
    set(pyexe "${ARG_PYTHON_EXECUTABLE}")
  endif()
  if(replace_missing_imports)
    string(SHA256 suffix_dir "${ARG_SOURCE}")
    set(temp_dir "$ENV{TMPDIR}")
    if(NOT temp_dir)
      # create a random dir in the build tree
      set(temp_dir "${CMAKE_CURRENT_BINARY_DIR}/${suffix_dir}")
    else()
      set(temp_dir "${temp_dir}/${suffix_dir}")
    endif()
    # create the temp dir (if it does not exist)
    if(NOT EXISTS "${temp_dir}")
      file(MAKE_DIRECTORY "${temp_dir}")
    endif()
    get_filename_component(filename "${ARG_SOURCE}" NAME)
    set(temp_file "${temp_dir}/${filename}")
    execute_process(COMMAND ${pyexe} "${CMAKE_CURRENT_SOURCE_DIR}/transform_ast.py" -o
                            "${temp_file}" "${ARG_SOURCE}" "${ARG_MODULES}")
    set(file_to_check "${temp_file}")
  else()
    set(file_to_check "${ARG_SOURCE}")
  endif()
  # run pytest on that temp file. TODO figure out if we need to run the collection from a certain
  # dir, and if we need conftest.py
  execute_process(
    COMMAND ${pyexe} -m pytest --collect-only -q "${file_to_check}"
    RESULT_VARIABLE pytest_success
    OUTPUT_VARIABLE pytest_tests
    ERROR_VARIABLE pytest_errorlog)
  if(NOT pytest_success EQUAL 0)
    message(
      FATAL_ERROR "Pytest collection failed for ${ARG_SOURCE}; error log is:\n${pytest_errorlog}")
  endif()
  # we need to remove the last 2 lines because pytest is overly verbose
  string(REGEX REPLACE "\n[^\n]*\n[^\n]*$" "" pytest_tests "${pytest_tests}")
  # Split into list by newlines
  string(REPLACE "\n" ";" pytest_tests "${pytest_tests}")
  # Pytest outputs each test on a single line, with the format: <filename>::<test> We need to remove
  # the <filename> and the first 2 colons.
  set(cleaned_tests)
  foreach(test_line IN LISTS pytest_tests)
    # Remove everything up to and including the first ::
    string(REGEX REPLACE "^[^:]+::" "" test_name "${test_line}")
    list(APPEND cleaned_tests "${test_name}")
  endforeach()
  set(pytest_tests "${cleaned_tests}")
  # Now we actually add the test. Note that the file may not exist at configure-time, but it must
  # exist at runtime of the test.
  foreach(test IN LISTS pytest_tests)
    add_test(
      NAME "${ARG_TARGET}::${test}"
      COMMAND ${pyexe} -m pytest ${ARG_PYTEST_ARGS} -k "${test}" "${destination}"
      WORKING_DIRECTORY "${working_directory}")
  endforeach()
endfunction()
