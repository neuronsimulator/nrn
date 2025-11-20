# Function for adding a test _at configure-time_ (because CTest does not work otherwise) using
# Pytest.
function(pytest_add_test)
  set(options REPLACE_MISSING_IMPORTS)
  set(oneValueArgs TARGET PYTHON_EXECUTABLE SOURCE DESTINATION WORKING_DIRECTORY)
  # the list of modules to remove from the AST (since they are most likely imported)
  set(multiValueArgs MODULES PYTEST_ARGS ENVIRONMENT)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT ARG_SOURCE)
    message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION}: You must provide the `SOURCE` parameter")
  endif()
  if(NOT ARG_DESTINATION)
    # if no destination, assume it is the current file
    set(destination "${ARG_SOURCE}")
  else()
    set(destination "${ARG_DESTINATION}")
  endif()
  if(NOT ARG_WORKING_DIRECTORY)
    # wherever will the current file be at runtime
    get_filename_component(working_directory "${destination}" DIRECTORY)
  else()
    # we do not check it's actually a directory, because it may only exist at runtime
    set(working_directory "${ARG_WORKING_DIRECTORY}")
  endif()
  if(NOT ARG_REPLACE_MISSING_IMPORTS)
    set(replace_missing_imports OFF)
  else()
    if(NOT ARG_MODULES)
      message(
        FATAL_ERROR
          "${CMAKE_CURRENT_FUNCTION}: With `REPLACE_MISSING_IMPORTS`, you must provide at least one module to the `MODULES` parameter"
      )
    endif()
    set(replace_missing_imports ON)
  endif()
  if(NOT ARG_PYTHON_EXECUTABLE)
    # inform the user we are guessing the Python version (with a sensible min)
    set(python_min_version "3.9")
    message(
      WARNING
        "${CMAKE_CURRENT_FUNCTION}: No value for `PYTHON_EXECUTABLE`; using `find_package` naively with min version ${python_min_version}"
    )
    find_package(Python "${python_min_version}" REQUIRED)
    set(pyexe "${Python_EXECUTABLE}")
  else()
    set(pyexe "${ARG_PYTHON_EXECUTABLE}")
  endif()
  # check Pytest is available
  execute_process(
    COMMAND ${pyexe} -m pytest --help
    RESULT_VARIABLE pytest_found
    OUTPUT_QUIET ERROR_QUIET)
  if(NOT pytest_found EQUAL 0)
    message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION}: pytest not found for executable ${pyexe}")
  endif()
  if(NOT ARG_ENVIRONMENT)
    set(command ${pyexe} -m pytest)
  else()
    set(command
        ${CMAKE_COMMAND}
        -E
        env
        ${ARG_ENVIRONMENT}
        ${pyexe}
        -m
        pytest)
  endif()
  get_filename_component(filename "${ARG_SOURCE}" NAME)
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
    set(temp_file "${temp_dir}/${filename}")
    execute_process(
      COMMAND ${pyexe} "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/transform_ast.py" -o "${temp_file}"
              "${ARG_SOURCE}" ${ARG_MODULES}
      RESULT_VARIABLE transform_success
      ERROR_VARIABLE transform_errorlog)
    if(NOT transform_success EQUAL 0)
      message(
        FATAL_ERROR
          "${CMAKE_CURRENT_FUNCTION}: AST transformation failed, the log is:\n${transform_errorlog}"
      )
    endif()
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
  # edge case: pytest collected no tests for the file, so we just output a warning
  if(pytest_success EQUAL 5)
    message(WARNING "No pytest tests were found in ${ARG_SOURCE}, skipping it...")
    return()
  elseif(NOT pytest_success EQUAL 0)
    message(
      FATAL_ERROR
        "${CMAKE_CURRENT_FUNCTION}: Pytest collection failed for ${ARG_SOURCE}; stdout is:\n${pytest_tests}\nerror log is:\n${pytest_errorlog}"
    )
  endif()
  if(replace_missing_imports)
    # remove the temp dir once we run Pytest on the file (and it succeeded)
    file(REMOVE_RECURSE "${temp_dir}")
  endif()
  # Split into list by newlines
  string(REPLACE "\n" ";" pytest_tests "${pytest_tests}")
  # Pytest outputs each test on a single line, with the format: <filename>::<test> We need to remove
  # the <filename> and the first 2 colons. We also need to remove any lines not matching the
  # filename (because Pytest is overly verbose)
  set(cleaned_tests)
  foreach(test_line IN LISTS pytest_tests)
    if(test_line AND test_line MATCHES "${filename}")
      # Remove everything up to and including the first ::
      string(REGEX REPLACE "^[^:]+::" "" test_name "${test_line}")
      list(APPEND cleaned_tests "${test_name}")
    endif()
  endforeach()
  set(pytest_tests "${cleaned_tests}")
  # Now we actually add the test. Note that the file may not exist at configure-time, but it must
  # exist at runtime of the test.
  foreach(test IN LISTS pytest_tests)
    set(test_name "${ARG_TARGET}::${test}")
    message(STATUS "Adding test ${test_name}")
    add_test(
      NAME "${test_name}"
      COMMAND ${command} ${ARG_PYTEST_ARGS} -k "${test}" "${destination}"
      WORKING_DIRECTORY "${working_directory}")
  endforeach()
endfunction()
