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

# For each Python in NRN_PYTHON_EXECUTABLES, find its version number, it's include directory, and
# its library path. Store those in the new lists NRN_PYTHON_VERSIONS, NRN_PYTHON_INCLUDES and
# NRN_PYTHON_LIBRARIES. Set NRN_PYTHON_COUNT to be the length of those lists, and
# NRN_PYTHON_ITERATION_LIMIT to be NRN_PYTHON_COUNT - 1.
set(NRN_PYTHON_EXECUTABLES)
set(NRN_PYTHON_VERSIONS)
set(NRN_PYTHON_INCLUDES)
set(NRN_PYTHON_LIBRARIES)
foreach(pyexe ${python_executables})
  message(STATUS "Checking if ${pyexe} is a working python")
  if(NOT IS_ABSOLUTE "${pyexe}")
    # Find the full path to ${pyexe} as Python3_EXECUTABLE does not accept relative paths.
    find_program(
      "${pyexe}_full" "${pyexe}"
      PATHS ENV PATH
      NO_DEFAULT_PATH)
    if(${pyexe}_full STREQUAL "${pyexe}_full-NOTFOUND")
      message(FATAL_ERROR "Could not resolve ${pyexe} to an absolute path")
    endif()
    set(pyexe "${${pyexe}_full}")
  endif()
  # Only bother finding version/include/library information if NRN_ENABLE_PYTHON is set.
  if(NRN_ENABLE_PYTHON)
    # Run find_package(Python3 ...) in a subprocess, so there is no pollution of CMakeCache.txt and
    # so on. Our desire to include multiple Python versions in one build means we have to handle
    # lists of versions/libraries/... manually. Unfortunately one cannot safely use find_package in
    # CMake script mode, so we configure an extra project.
    string(SHA1 pyexe_hash "${pyexe}")
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
        ${CMAKE_COMMAND} "-DPython3_EXECUTABLE:STRING=${pyexe}"
        "-DPython3_COMPONENTS=${dev_component};Interpreter" -S
        ${CMAKE_SOURCE_DIR}/cmake/ExecuteFindPython -B
        ${CMAKE_BINARY_DIR}/ExecuteFindPython_${pyexe_hash}
      RESULT_VARIABLE result
      OUTPUT_VARIABLE stdout
      ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0)
      message(FATAL_ERROR "find_package could not discover information about ${pyexe}\n"
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
    if(${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR} VERSION_LESS NRN_MINIMUM_PYTHON_VERSION)
      message(
        FATAL_ERROR
          "${pyexe} (${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}) is too old for NEURON, which requires at least ${NRN_MINIMUM_PYTHON_VERSION}"
      )
    endif()
    # Now Python3_INCLUDE_DIRS and friends correspond to ${pyexe} Assert that there is only one
    # value per Python version for now, as otherwise we'd need to handle a list of lists...
    list(LENGTH Python3_INCLUDE_DIRS num_include_dirs)
    if(NOT num_include_dirs EQUAL 1)
      message(
        FATAL_ERROR
          "Cannot currently handle multiple Python include dirs: ${Python3_INCLUDE_DIRS} from ${pyexe}"
      )
    endif()
    list(LENGTH Python3_LIBRARIES num_libs)
    if(NOT num_libs EQUAL 1)
      message(
        FATAL_ERROR
          "Cannot currently handle multiple Python libraries: ${Python3_LIBRARIES} from ${pyexe}")
    endif()
    list(APPEND NRN_PYTHON_VERSIONS "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
    list(APPEND NRN_PYTHON_INCLUDES "${Python3_INCLUDE_DIRS}")
    list(APPEND NRN_PYTHON_LIBRARIES "${Python3_LIBRARIES}")
  endif()
  # One final transformation, for convenience when using macOS and sanitizers that need to have
  # their runtime libraries loaded, replace any shims with the real Python executable.
  cpp_cc_strip_python_shims(EXECUTABLE "${pyexe}" OUTPUT pyexe)
  list(APPEND NRN_PYTHON_EXECUTABLES "${pyexe}")
endforeach()
# In any case, the default (NRN_DEFAULT_PYTHON_EXECUTABLE) should always be the zeroth entry in the
# list of Pythons, and we need to set it even if NRN_ENABLE_PYTHON=OFF -- for use in build scripts.
list(GET NRN_PYTHON_EXECUTABLES 0 NRN_DEFAULT_PYTHON_EXECUTABLE)
if(NRN_ENABLE_PYTHON)
  list(GET NRN_PYTHON_INCLUDES 0 NRN_DEFAULT_PYTHON_INCLUDES)
  list(GET NRN_PYTHON_LIBRARIES 0 NRN_DEFAULT_PYTHON_LIBRARIES)
  list(LENGTH NRN_PYTHON_EXECUTABLES NRN_PYTHON_COUNT)
  math(EXPR NRN_PYTHON_ITERATION_LIMIT "${NRN_PYTHON_COUNT} - 1")
endif()
