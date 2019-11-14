# =============================================================================
# Configure support for dynamic Python to use multiple Python versions
# =============================================================================
#
# NEURON can be built with python modules that can be usable from multiple
# versions of Python. Here we check if NRN_ENABLE_PYTHON_DYNAMIC is valid
# and determine an include directory for version 2 and/or 3 to build
# libnrnpython<major>.so. Depending on the pythons used, either or both of
# NRNPYTHON_INCLUDE3 or NRNPYTHON_INCLUDE2 will be defined.

# The above is good for mac and linux. Sadly, for MINGW, a distinct
# NRNPYTHON_INCLUDE<major><minor> is needed for each python in the
# NRN_PYTHON_DYNAMIC list. This is because libnrnpython<major><minor>.dll
# must be linked against the specfic libpython to avoid undefined name errors.
# Thus, at least for MINGW, parallel to the NRN_PYTHON_DYNAMIC list
# we construct the lists NRN_PYTHON_VER_LIST, NRN_PYTHON_INCLUDE_LIST,
# and NRN_PYTHON_LIB_LIST

set(LINK_AGAINST_PYTHON ${MINGW})
set(NRN_PYTHON_VER_LIST "" CACHE INTERNAL "" FORCE)
set(NRN_PYTHON_INCLUDE_LIST "" CACHE INTERNAL "" FORCE)
set(NRN_PYTHON_LIB_LIST "" CACHE INTERNAL "" FORCE)

# Inform setup.py and nrniv/nrnpy.cpp whether libnrnpython name is libnrnpython<major>
# or libnrnpython<major><minor> . The latter is required for mingw.
# This is here instead of in src/nrnpython/CMakeLists.txt as src/nrniv/CMakeLists
# needs it for nrniv/nrnpy.cpp
set(USE_LIBNRNPYTHON_MAJORMINOR 0)
if (LINK_AGAINST_PYTHON)
  set(USE_LIBNRNPYTHON_MAJORMINOR 1)
endif()

if (NRN_ENABLE_PYTHON)
  if (NRN_ENABLE_PYTHON_DYNAMIC)
    if ("${NRN_PYTHON_DYNAMIC}" STREQUAL "")
      # use the default python already determined
      if (LINK_AGAINST_PYTHON)
        set(PYVER "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
      else()
        set(PYVER ${PYTHON_VERSION_MAJOR})
      endif()
      # NB: we are constructing here a variable name NRNPYTHON_INCLUDE${PYVER}
      set(NRNPYTHON_INCLUDE${PYVER} ${PYTHON_INCLUDE_DIRS})
      list(APPEND NRN_PYTHON_VER_LIST "${PYVER}")
      list(APPEND NRN_PYTHON_INCLUDE_LIST "${PYTHON_INCLUDE_DIRS}")
      list(APPEND NRN_PYTHON_LIB_LIST "${PYTHON_LIBRARIES}")
    else()
      # run each python provided by user to determine major and include directory
      message(STATUS "Dynamic Python support")
      foreach(pyexe ${NRN_PYTHON_DYNAMIC})
        message(STATUS "Checking if ${pyexe} is a working python")
        if (LINK_AGAINST_PYTHON)
          set (pr_pyver "print('%d.%d' % (sys.version_info[0], sys.version_info[1]))")
        else()
          set (pr_pyver "print(sys.version_info[0])")
        endif()
        execute_process(COMMAND ${pyexe} -c "from distutils.sysconfig import get_python_inc as p; print(p()); import sys; ${pr_pyver}; quit()"
          RESULT_VARIABLE result
          OUTPUT_VARIABLE std_output
          ERROR_VARIABLE  err_output
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(result EQUAL 0)
          string(REGEX MATCH [0-9.]*$ PYVER ${std_output})
          string(REGEX MATCH ^[^\n]* incval ${std_output})
          if (NOT NRNPYTHON_INCLUDE${PYVER})
            set (NRNPYTHON_INCLUDE${PYVER} ${incval})
          endif()
          # Only needed to find include and library paths if LINK_AGAINST_PYTHON but useful
          # for build report. Unset the variables set by PythonLibsNew so we can start afresh.
          set(PYTHON_EXECUTABLE ${pyexe})
          unset(PYTHON_INCLUDE_DIR CACHE)
          unset(PYTHON_LIBRARY CACHE)
          set(PYTHON_PREFIX "")
          set(PYTHON_LIBRARIES "")
          set(PYTHON_INCLUDE_DIRS "")
          set(PYTHON_MODULE_EXTENSION "")
          set(PYTHON_MODULE_PREFIX "")

          find_package(PythonLibsNew ${PYVER} REQUIRED)
          # convert major.minor to majorminor
          string(REGEX REPLACE [.] "" PYVER ${PYVER})
          list(APPEND NRN_PYTHON_VER_LIST "${PYVER}")
          list(APPEND NRN_PYTHON_INCLUDE_LIST "${incval}")
          list(APPEND NRN_PYTHON_LIB_LIST "${PYTHON_LIBRARIES}")
        else()
          message(FATAL_ERROR "Error while checking ${pyexe} : ${result}\n${std_output}\n${err_output}")
        endif()
      endforeach()
    endif()
  endif()
endif()

# check Python.h exists under provided include directory
macro(check_python_include python_include major_version)
  if (${python_include})
    if (NOT EXISTS "${${python_include}}/Python.h")
      message(FATAL_ERROR " ${${python_include}}/Python.h does not exist, set proper ${python_include} to include directory")
    endif()
  endif()
endmacro()

if (NOT LINK_AGAINST_PYTHON)
  # make sure provided python have Python.h header
  check_python_include(NRNPYTHON_INCLUDE2 2)
  check_python_include(NRNPYTHON_INCLUDE3 3)
endif()
