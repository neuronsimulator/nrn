# * Macro to find a python module
#
# Usage: find_python_module (module [VERSION] [REQUIRED])
#
# Copyright 2005-2018 Airbus-EDF-IMACS-Phimeca
#
# Distributed under the OSI-approved BSD License (the "License"); see accompanying file
# Copyright.txt for details in:
#
# https://github.com/openturns/otsubsetinverse/blob/master/cmake/FindPythonModule.cmake

macro(find_python_module module)

  string(TOUPPER ${module} module_upper)
  if(NOT ${module_upper}_FOUND)

    # parse arguments
    set(${module}_FIND_OPTIONAL TRUE)
    if(${ARGC} EQUAL 2)
      if(${ARGV1} MATCHES REQUIRED)
        set(${module}_FIND_OPTIONAL FALSE)
      else()
        set(${module}_FIND_VERSION ${ARGV1})
      endif()
    elseif(${ARGC} EQUAL 3)
      if(${ARGV2} MATCHES REQUIRED)
        set(${module}_FIND_OPTIONAL FALSE)
      endif()
      set(${module}_FIND_VERSION ${ARGV1})
    endif()

    # A module's location is usually a directory, but for binary modules it's a .so file.
    execute_process(
      COMMAND "${PYTHON_EXECUTABLE}" "-c"
              "import re, ${module}; print(re.compile('/__init__.py.*').sub('',${module}.__file__))"
      RESULT_VARIABLE _${module}_status
      OUTPUT_VARIABLE _${module}_location
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _${module}_status)
      set(${module_upper}_LOCATION ${_${module}_location}
          CACHE STRING "Location of Python module ${module}")
      # retrieve version
      execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
                              "import ${module}; print(${module}.__version__)"
                      RESULT_VARIABLE _${module}_status
                      OUTPUT_VARIABLE _${module}_version
                      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

      set(_${module_upper}_VERSION_MATCH TRUE)
      if(NOT _${module}_status)
        set(${module_upper}_VERSION_STRING ${_${module}_version})
        if(${module}_FIND_VERSION)
          if(${module}_FIND_VERSION VERSION_GREATER ${module_upper}_VERSION_STRING)
            set(_${module_upper}_VERSION_MATCH FALSE)
          endif()
        endif()
        mark_as_advanced(${module_upper}_VERSION_STRING)
      endif()
    endif()

    find_package_handle_standard_args(${module}
                                      REQUIRED_VARS
                                      ${module_upper}_LOCATION
                                      _${module_upper}_VERSION_MATCH
                                      VERSION_VAR
                                      ${module_upper}_VERSION_STRING)
    if(NOT ${module}_FIND_OPTIONAL AND NOT _${module_upper}_VERSION_MATCH)
      message(FATAL_ERROR "Missing python module ${module}")
    endif()
    mark_as_advanced(${module_upper}_LOCATION)
  endif(NOT ${module_upper}_FOUND)
endmacro(find_python_module)
