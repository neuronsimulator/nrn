# Usage: nrn_find_python_module(MODULE module [VERSION version] [REQUIRED] [ALL])
#
# * MODULE: specifies the Python module to try and import
# * ALL: if specified, checks that the module exists in *all* of the Python executables
#   ${NRN_PYTHON_EXECUTABLES}, otherwise checks ${NRN_DEFAULT_PYTHON_EXECUTABLE}
# * REQUIRED: if specified, emit a FATAL_ERROR if the module cannot be found
# * VERSION: if specified, require the package version is at least `version`
#
# Sets UPPERCASEMODULENAME_FOUND if the module was found.
#
# Extensively modified for NEURON, but original license information below:
#
# Copyright 2005-2018 Airbus-EDF-IMACS-Phimeca
#
# Distributed under the OSI-approved BSD License (the "License"); see accompanying file
# Copyright.txt for details in:
#
# https://github.com/openturns/otsubsetinverse/blob/master/cmake/FindPythonModule.cmake

macro(nrn_find_python_module)
  cmake_parse_arguments(_find_module "ALL;REQUIRED" "MODULE;VERSION" "" ${ARGN})
  if(NOT _find_module_MODULE)
    message(FATAL_ERROR "The MODULE option is required")
  endif()
  # Which Pythons are we going to check
  if(_find_module_ALL)
    set(_find_module_pythons ${NRN_PYTHON_EXECUTABLES})
  else()
    set(_find_module_pythons "${NRN_DEFAULT_PYTHON_EXECUTABLE}")
  endif()
  # Reset these (we're a macro, after all)
  set(_find_module_versions)
  set(_find_module_not_found)
  set(_find_module_insufficient_version)
  # Loop through Pythons, track lists of which Pythons had missing/insufficient versions
  foreach(_find_module_python ${_find_module_pythons})
    # A module's location is usually a directory, but for binary modules it's a .so file.
    string(JOIN "; " _find_module_python_code "import re, ${_find_module_MODULE}"
           "print(re.compile('/__init__.py.*').sub('',${_find_module_MODULE}.__file__))")
    execute_process(
      COMMAND "${_find_module_python}" "-c" "${_find_module_python_code}"
      RESULT_VARIABLE _find_module_status
      OUTPUT_VARIABLE _find_module_location
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_find_module_status)
      # module not found with this Python
      list(APPEND _find_module_not_found "${_find_module_python}")
      # keep going for the sake of a good error
      continue()
    endif()
    # module *was* found with this Python; retrieve version
    execute_process(
      COMMAND "${_find_module_python}" "-c"
              "import ${_find_module_MODULE}; print(${_find_module_MODULE}.__version__)"
      RESULT_VARIABLE _find_module_status
      OUTPUT_VARIABLE _find_module_version
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_find_module_status)
      # module version could not be determined, some packages don't define __version__ if VERSION
      # was passed, we may need to fail because of this
      if(_find_module_VERSION)
        list(APPEND _find_module_insufficient_version "${_find_module_python}-UNKNOWN")
      endif()
      # keep going, this may not be fatal
      continue()
    endif()
    # module version is in _find_module_version
    list(APPEND _find_module_versions "${_find_module_version}")
    if(_find_module_VERSION AND _find_module_VERSION VERSION_GREATER "${_find_module_version}")
      # a minimum version was specified, and it wasn't met with this Python
      list(APPEND _find_module_insufficient_version
           "${_find_module_python}-${_find_module_version}")
      # keep going for the sake of a good error
      continue()
    endif()
  endforeach()
  string(TOUPPER "${_find_module_MODULE}" _find_module_upper)
  if(_find_module_not_found OR _find_module_insufficient_version)
    set("${_find_module_upper}_FOUND" OFF)
    if(_find_module_REQUIRED)
      set(_find_module_message_type FATAL_ERROR)
    else()
      set(_find_module_message_type STATUS)
    endif()
    # Print a message explaining why the module wasn't found with the given constraints
    set(_find_module_msg "Could not find Python module '${_find_module_MODULE}'")
    if(_find_module_not_found)
      string(JOIN " " _find_module_msg "${_find_module_msg}" "with:" ${_find_module_not_found})
    endif()
    if(_find_module_insufficient_version)
      string(JOIN " " _find_module_msg "${_find_module_msg}" "insufficient version with:"
             ${_find_module_insufficient_version})
    endif()
    message(${_find_module_message_type} "${_find_module_msg}")
  else()
    set("${_find_module_upper}_FOUND" ON)
    # Print a quick summary of what we found with which versions: _find_module_versions
    list(REMOVE_DUPLICATES _find_module_versions)
    set(_find_module_msg "Found Python module '${_find_module_MODULE}'")
    if(_find_module_versions)
      string(JOIN " " _find_module_msg ${_find_module_msg} "with version(s):"
             ${_find_module_versions})
    endif()
    message(STATUS "${_find_module_msg}")
  endif()
endmacro(nrn_find_python_module)
