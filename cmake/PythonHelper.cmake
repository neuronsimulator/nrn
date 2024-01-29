# =================================================================================================
# Organise which Python versions are to be built against, and find their versions, include
# directories and library paths. This is used both for dynamic Python (>= 1 libnrnpythonX.Y) and
# standard Python (libnrniv linked against one Python version) builds. To avoid the restrictions
# inherent in Python's limited API / stable ABI (see
# https://docs.python.org/3/c-api/stable.html#stable-application-binary-interface), we build
# Python-related NEURON code separately for each version of Python: libnrnpythonX.Y. Historically
# macOS and Linux were built ignoring the minor version, but this is unsafe without the limited API
# =================================================================================================

# Parse commandline options so that:
#
# * NRN_DEFAULT_PYTHON_EXECUTABLE is the default Python, which is used for running tests and so on.
# * NRN_PYTHON_EXECUTABLES is a list of all the Pythons that we are building against. This will only
#   have a length > 1 if NRN_ENABLE_PYTHON_DYNAMIC is defined.
if(NOT PYTHON_EXECUTABLE AND (NOT NRN_ENABLE_PYTHON_DYNAMIC OR NOT NRN_PYTHON_DYNAMIC))
  # Haven't been explicitly told about any Python versions, set PYTHON_EXECUTABLE by searching PATH
  message(STATUS "No python executable specified. Looking for `python3` in the PATH...")
  # Since PythonInterp module prefers system-wide python, if PYTHON_EXECUTABLE is not set, look it
  # up in the PATH exclusively. Need to set PYTHON_EXECUTABLE before calling SanitizerHelper.cmake
  find_program(
    PYTHON_EXECUTABLE python3
    PATHS ENV PATH
    NO_DEFAULT_PATH)
  if(PYTHON_EXECUTABLE STREQUAL "PYTHON_EXECUTABLE-NOTFOUND")
    message(FATAL_ERROR "Could not find Python, please set PYTHON_EXECUTABLE or NRN_PYTHON_DYNAMIC")
  endif()
endif()
if(NRN_ENABLE_PYTHON_DYNAMIC AND NRN_PYTHON_DYNAMIC)
  list(GET NRN_PYTHON_DYNAMIC 0 NRN_PYTHON_DYNAMIC_0)
  if(PYTHON_EXECUTABLE AND NOT PYTHON_EXECUTABLE STREQUAL NRN_PYTHON_DYNAMIC_0)
    # When NRN_ENABLE_PYTHON_DYNAMIC and NRN_PYTHON_DYNAMIC are set, the first entry of
    # NRN_PYTHON_DYNAMIC is taken to be the default python version
    message(WARNING "Default python version is ${NRN_PYTHON_DYNAMIC_0} (from NRN_PYTHON_DYNAMIC),"
                    " not ${PYTHON_EXECUTABLE} (from PYTHON_EXECUTABLE),"
                    " because NRN_ENABLE_PYTHON_DYNAMIC=ON")
  endif()
  set(python_executables ${NRN_PYTHON_DYNAMIC})
else()
  # In other cases, there is just one Python and it's PYTHON_EXECUTABLE.
  set(python_executables "${PYTHON_EXECUTABLE}")
endif()

# Given a name (e.g. python3.11) find the include directories, full executable path, libraries and
# version information.
# ~~~
# Usage: nrn_find_python(NAME python3.11 PREFIX nrnpy)
# ~~~
# Sets:
#
# * nrnpy_EXECUTABLE
# * nrnpy_INCLUDES
# * nrnpy_LIBRARIES
# * nrnpy_VERSION_MAJOR
# * nrnpy_VERSION_MINOR
#
# If NRN_ENABLE_PYTHON is *not* set then only nrnpy_EXECUTABLE will be set. There is some special
# handling on macOS when sanitizers are enabled:
#
# * if the Python executable does *not* belong to a virtual environment but *is* a shim (as is often
#   the case with binaries like /usr/bin/python on macOS) then nrnpy_EXECUTABLE will be set to the
#   real (non-shim) binary.
# * if the Python executable *does* point to a virtual environment that was configured using a
#   Python shim, an error is emitted with advice on how to re-create the virtual environment using
#   the real (non-shim) binary.
function(nrn_find_python)
  set(oneVal NAME PREFIX)
  cmake_parse_arguments(opt "" "${oneVal}" "" ${ARGN})
  if(opt_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unexpected arguments: ${opt_UNPARSED_ARGUMENTS}")
  endif()
  if(opt_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR "${opt_KEYWORDS_MISSING_VALUES} values are required")
  endif()
  if(NOT IS_ABSOLUTE "${opt_NAME}")
    # Find the full path to ${opt_NAME} as Python3_EXECUTABLE does not accept relative paths.
    find_program(
      "${opt_NAME}_full" "${opt_NAME}"
      PATHS ENV PATH
      NO_DEFAULT_PATH)
    if(${opt_NAME}_full STREQUAL "${opt_NAME}_full-NOTFOUND")
      set("${opt_PREFIX}_EXECUTABLE"
          "${opt_PREFIX}_EXECUTABLE-NOTFOUND"
          PARENT_SCOPE)
      return()
    endif()
    set(opt_NAME "${${opt_NAME}_full}")
  endif()
  # Only bother finding version/include/library information if NRN_ENABLE_PYTHON is set.
  if(NRN_ENABLE_PYTHON)
    # Run find_package(Python3 ...) in a subprocess, so there is no pollution of CMakeCache.txt and
    # so on. Our desire to include multiple Python versions in one build means we have to handle
    # lists of versions/libraries/... manually. Unfortunately one cannot safely use find_package in
    # CMake script mode, so we configure an extra project.
    string(SHA1 pyexe_hash "${opt_NAME}")
    string(SUBSTRING "${pyexe_hash}" 0 6 pyexe_hash)
    # Which attributes we're trying to learn about this Python
    set(python_vars Python3_INCLUDE_DIRS Python3_VERSION_MAJOR Python3_VERSION_MINOR)
    if(NRN_ENABLE_PYTHON_DYNAMIC AND NOT NRN_LINK_AGAINST_PYTHON)
      # Do not link against Python, so we don't need the library -- just as well, it's not available
      # in manylinux
      if(${CMAKE_VERSION} VERSION_LESS 3.18)
        message(
          FATAL_ERROR
            "NRN_ENABLE_PYTHON_DYNAMIC=ON and NRN_LINK_AGAINST_PYTHON=OFF requires CMake >= 3.18 for the Development.Module component in FindPython"
        )
      endif()
      set(dev_component "Development.Module")
      set(Python3_LIBRARIES "do-not-link-against-libpython-in-dynamic-python-builds")
    else()
      set(dev_component "Development")
      list(APPEND python_vars Python3_LIBRARIES)
    endif()
    execute_process(
      COMMAND
        ${CMAKE_COMMAND} "-DPython3_EXECUTABLE:STRING=${opt_NAME}"
        "-DPython3_COMPONENTS=${dev_component};Interpreter" -S
        ${CMAKE_SOURCE_DIR}/cmake/ExecuteFindPython -B
        ${CMAKE_BINARY_DIR}/ExecuteFindPython_${pyexe_hash}
      RESULT_VARIABLE result
      OUTPUT_VARIABLE stdout
      ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0)
      message(FATAL_ERROR "find_package could not discover information about ${opt_NAME}\n"
                          "status=${result}\n" "stdout:\n${stdout}\n" "stderr:\n${stderr}\n")
    endif()
    # Parse out the variables printed by ExecuteFindPython.cmake
    foreach(var ${python_vars})
      string(REGEX MATCH "-- ${var}=([^\n]*)\n" _junk "${stdout}")
      if(NOT _junk OR NOT CMAKE_MATCH_1)
        message(FATAL_ERROR "Could not extract ${var} from\n===\n${stdout}\n===")
      endif()
      set(${var} "${CMAKE_MATCH_1}")
    endforeach()
    set("${opt_PREFIX}_INCLUDES"
        "${Python3_INCLUDE_DIRS}"
        PARENT_SCOPE)
    set("${opt_PREFIX}_LIBRARIES"
        "${Python3_LIBRARIES}"
        PARENT_SCOPE)
    set("${opt_PREFIX}_VERSION_MAJOR"
        "${Python3_VERSION_MAJOR}"
        PARENT_SCOPE)
    set("${opt_PREFIX}_VERSION_MINOR"
        "${Python3_VERSION_MINOR}"
        PARENT_SCOPE)
  endif()
  # Finally do our special treatment for macOS + sanitizers
  if(APPLE AND NRN_SANITIZERS)
    # Detect if the binary we have in opt_NAME points to a virtual environment.
    execute_process(
      COMMAND "${opt_NAME}" -c "import sys; print(sys.prefix != sys.base_prefix)"
      RESULT_VARIABLE code
      OUTPUT_VARIABLE isvenv_str
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(code EQUAL 0 AND isvenv_str STREQUAL "True")
      # opt_NAME points into a virtual environment. This will only work with sanitizers if the
      # {venv}/bin/python symlink does *not* point to a shim.
      file(REAL_PATH "${opt_NAME}" pyexe_without_symlinks)
      cpp_cc_strip_python_shims(EXECUTABLE "${pyexe_without_symlinks}" OUTPUT deshimmed)
      if(NOT deshimmed STREQUAL pyexe_without_symlinks)
        # this is the sad case: a virtual environment sitting on top of a shim
        message(
          FATAL_ERROR
            "${opt_NAME} points into a virtual environment that was configured using a Python "
            "shim. This will not work with sanitizers enabled on macOS.\nTry re-creating your "
            "virtual environment using the real Python binary:\n"
            "PYTHONEXECUTABLE=${deshimmed} ${deshimmed} -mvenv new_venv_path")
      endif()
      # the virtual environment sits on top of the real (non-shimmed) Python, so it should all work:
      # opt_NAME is correct
    elseif(code EQUAL 0 AND isvenv_str STREQUAL "False")
      # opt_NAME does not point into a virtual environment, so we can safely strip out any shims
      cpp_cc_strip_python_shims(EXECUTABLE "${opt_NAME}" OUTPUT opt_NAME)
    else()
      message(FATAL_ERROR "Could not determine if ${opt_NAME} points into a virtual environment "
                          "(code=${code} isvenv_str=${isvenv_str})")
    endif()
  endif()
  set("${opt_PREFIX}_EXECUTABLE"
      "${opt_NAME}"
      PARENT_SCOPE)
endfunction()

# For each Python in NRN_PYTHON_EXECUTABLES, find its version number, its include directory, and its
# library path. Store those in the new lists NRN_PYTHON_VERSIONS, NRN_PYTHON_INCLUDES and
# NRN_PYTHON_LIBRARIES. Set NRN_PYTHON_COUNT to be the length of those lists, and
# NRN_PYTHON_ITERATION_LIMIT to be NRN_PYTHON_COUNT - 1.
set(NRN_PYTHON_EXECUTABLES)
set(NRN_PYTHON_VERSIONS)
set(NRN_PYTHON_INCLUDES)
set(NRN_PYTHON_LIBRARIES)
foreach(pyexe ${python_executables})
  message(STATUS "Checking if ${pyexe} is a working python")
  nrn_find_python(NAME "${pyexe}" PREFIX nrnpy)
  if(NRN_ENABLE_PYTHON)
    # If NRN_ENABLE_PYTHON=OFF then we're only using Python to run build scripts etc.
    set(nrnpy_VERSION "${nrnpy_VERSION_MAJOR}.${nrnpy_VERSION_MINOR}")
    if(${nrnpy_VERSION} VERSION_LESS NRN_MINIMUM_PYTHON_VERSION)
      message(FATAL_ERROR "${pyexe} too old (${nrnpy_VERSION} < ${NRN_MINIMUM_PYTHON_VERSION})")
    endif()
    # Now nrnpy_INCLUDES and friends correspond to ${pyexe}. Assert that there is only one value per
    # Python version for now, as otherwise we'd need to handle a list of lists...
    list(LENGTH nrnpy_INCLUDES num_include_dirs)
    list(LENGTH nrnpy_LIBRARIES num_lib_dirs)
    if(NOT num_include_dirs EQUAL 1)
      message(FATAL_ERROR "Cannot handle multiple Python include dirs: ${nrnpy_INCLUDES}")
    endif()
    if(NOT num_lib_dirs EQUAL 1)
      message(FATAL_ERROR "Cannot handle multiple Python libraries: ${Python3_LIBRARIES}")
    endif()
    if(nrnpy_VERSION IN_LIST NRN_PYTHON_VERSIONS)
      # We cannot build against multiple copies of the same pythonX.Y version.
      message(FATAL_ERROR "Got duplicate version ${nrnpy_VERSION} from ${pyexe}")
    endif()
    list(APPEND NRN_PYTHON_VERSIONS "${nrnpy_VERSION}")
    list(APPEND NRN_PYTHON_INCLUDES "${nrnpy_INCLUDES}")
    list(APPEND NRN_PYTHON_LIBRARIES "${nrnpy_LIBRARIES}")
  endif()
  list(APPEND NRN_PYTHON_EXECUTABLES "${nrnpy_EXECUTABLE}")
endforeach()
# In any case, the default (NRN_DEFAULT_PYTHON_EXECUTABLE) should always be the zeroth entry in the
# list of Pythons, and we need to set it even if NRN_ENABLE_PYTHON=OFF -- for use in build scripts.
list(GET NRN_PYTHON_EXECUTABLES 0 NRN_DEFAULT_PYTHON_EXECUTABLE)
list(GET NRN_PYTHON_VERSIONS 0 NRN_DEFAULT_PYTHON_VERSION)
if(NRN_ENABLE_PYTHON)
  list(GET NRN_PYTHON_INCLUDES 0 NRN_DEFAULT_PYTHON_INCLUDES)
  list(GET NRN_PYTHON_LIBRARIES 0 NRN_DEFAULT_PYTHON_LIBRARIES)
  list(LENGTH NRN_PYTHON_EXECUTABLES NRN_PYTHON_COUNT)
  math(EXPR NRN_PYTHON_ITERATION_LIMIT "${NRN_PYTHON_COUNT} - 1")
endif()
if(NRN_ENABLE_TESTS AND NRN_ENABLE_PYTHON)
  # Make sure that, if NRN_PYTHON_EXTRA_FOR_TESTS is set, none of its versions clash with versions
  # we're building against
  set(NRN_PYTHON_EXTRA_FOR_TESTS_EXECUTABLES)
  set(NRN_PYTHON_EXTRA_FOR_TESTS_VERSIONS)
  foreach(pyexe ${NRN_PYTHON_EXTRA_FOR_TESTS})
    nrn_find_python(NAME "${pyexe}" PREFIX nrnpy)
    set(nrnpy_VERSION "${nrnpy_VERSION_MAJOR}.${nrnpy_VERSION_MINOR}")
    if(nrnpy_VERSION IN_LIST NRN_PYTHON_VERSIONS)
      string(JOIN ", " versions ${NRN_PYTHON_VERSIONS})
      message(FATAL_ERROR "NRN_PYTHON_EXTRA_FOR_TESTS=${NRN_PYTHON_EXTRA_FOR_TESTS} cannot contain"
                          " Python versions that NEURON *is* being built against (${versions})")
    endif()
    list(APPEND NRN_PYTHON_EXTRA_FOR_TESTS_EXECUTABLES "${nrnpy_EXECUTABLE}")
    list(APPEND NRN_PYTHON_EXTRA_FOR_TESTS_VERSIONS "${nrnpy_VERSION}")
  endforeach()
  list(LENGTH NRN_PYTHON_EXTRA_FOR_TESTS NRN_PYTHON_EXTRA_FOR_TESTS_COUNT)
  if(NRN_PYTHON_EXTRA_FOR_TESTS)
    math(EXPR NRN_PYTHON_EXTRA_FOR_TESTS_ITERATION_LIMIT "${NRN_PYTHON_EXTRA_FOR_TESTS_COUNT} - 1")
  endif()
endif()
