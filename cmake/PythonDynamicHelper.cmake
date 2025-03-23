# =============================================================================
# Configure support for dynamic Python to use multiple Python versions
# =============================================================================
# ~~~
# NEURON can be built with python modules that can be usable from multiple
# versions of Python. Here we check if NRN_ENABLE_PYTHON_DYNAMIC is valid
# and determine an include directory for version 3 to build
# libnrnpython<major>.so. For now only NRNPYTHON_INCLUDE3 will be defined.
#
# The above is good for mac and linux. Sadly, for MINGW, a distinct
# NRNPYTHON_INCLUDE<major><minor> is needed for each python in the
# NRN_PYTHON_DYNAMIC list. This is because libnrnpython<major><minor>.dll
# must be linked against the specfic libpython to avoid undefined name errors.
# Thus, at least for MINGW, parallel to the NRN_PYTHON_DYNAMIC list
# we construct the lists NRN_PYTHON_VER_LIST, NRN_PYTHON_INCLUDE_LIST,
# and NRN_PYTHON_LIB_LIST

# set(LINK_AGAINST_PYTHON ${MINGW}) sadly, the Python ABI seems to have changed for Python 3.13
# (possibly 3.12) and for 3.13 we experience libnrnpython3.so: undefined symbol:
# _PyObject_NextNotImplemented if <3.13 is first in the dynamic list. And abort on others if 3.13 is
# first. so
# ~~~

set(LINK_AGAINST_PYTHON ${NRN_ENABLE_PYTHON_DYNAMIC})

set(NRN_PYTHON_VER_LIST
    ""
    CACHE INTERNAL "" FORCE)
set(NRN_PYTHON_INCLUDE_LIST
    ""
    CACHE INTERNAL "" FORCE)
set(NRN_PYTHON_LIB_LIST
    ""
    CACHE INTERNAL "" FORCE)
set(NRN_PYTHON_VERSIONS
    ""
    CACHE INTERNAL "" FORCE)

# ~~~
# Inform setup.py and nrniv/nrnpy.cpp whether libnrnpython name is libnrnpython<major>
# or libnrnpython<major>.<minor> . The latter used to be required only for mingw.
# This is here instead of in src/nrnpython/CMakeLists.txt as src/nrniv/CMakeLists
# needs it for nrniv/nrnpy.cpp
# ~~~
set(USE_LIBNRNPYTHON_MAJORMINOR 0)
if(LINK_AGAINST_PYTHON)
  set(USE_LIBNRNPYTHON_MAJORMINOR 1)
endif()

if(NRN_ENABLE_PYTHON)
  if(NRN_ENABLE_PYTHON_DYNAMIC)
    if(NRN_PYTHON_DYNAMIC STREQUAL "")
      # use the default python already determined
      if(LINK_AGAINST_PYTHON)
        set(PYVER "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
      else()
        set(PYVER ${PYTHON_VERSION_MAJOR})
      endif()
      set(NRN_PYTHON_VERSIONS "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
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
        if(LINK_AGAINST_PYTHON)
          set(pr_pyver "print('%d.%d' % (sys.version_info[0], sys.version_info[1]))")
        else()
          set(pr_pyver "print(sys.version_info[0])")
        endif()
        execute_process(
          COMMAND
            ${pyexe} -c
            "import sysconfig; print(sysconfig.get_path('include')); import sys; ${pr_pyver};quit()"
          RESULT_VARIABLE result
          OUTPUT_VARIABLE std_output
          ERROR_VARIABLE err_output
          OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(result EQUAL 0)
          # cmake-format: off
          string(REGEX MATCH [0-9.]*$ PYVER ${std_output})
          string(REGEX MATCH ^[^\n]* incval ${std_output})
          # cmake-format: on
          if(NOT NRNPYTHON_INCLUDE${PYVER})
            set(NRNPYTHON_INCLUDE${PYVER} ${incval})
          endif()
          # Only needed to find include and library paths if LINK_AGAINST_PYTHON but useful for
          # build report. Unset the variables set by PythonLibsNew so we can start afresh.
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
          list(APPEND NRN_PYTHON_VER_LIST "${PYVER}")
          list(APPEND NRN_PYTHON_INCLUDE_LIST "${incval}")
          list(APPEND NRN_PYTHON_LIB_LIST "${PYTHON_LIBRARIES}")
        else()
          message(
            FATAL_ERROR "Error while checking ${pyexe} : ${result}\n${std_output}\n${err_output}")
        endif()

        # again to calculate PYVER (major.minor) for NRN_PYTHON_VERSIONS
        set(pr_pyverxy "print('%d.%d' % (sys.version_info[0], sys.version_info[1]))")
        execute_process(
          COMMAND ${pyexe} -c "import sys; ${pr_pyverxy}; quit()"
          RESULT_VARIABLE result
          OUTPUT_VARIABLE std_output
          ERROR_VARIABLE err_output
          OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(result EQUAL 0)
          # cmake-format: off
          string(REGEX MATCH [0-9.]*$ PYVERXY ${std_output})
          # cmake-format: on
          list(APPEND NRN_PYTHON_VERSIONS "${PYVERXY}")
        endif()
      endforeach()
    endif()
  else()
    # the default python version
    set(NRN_PYTHON_VERSIONS "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
  endif()
endif()

# check Python.h exists under provided include directory
macro(check_python_include python_include major_version)
  if(python_include)
    if(NOT EXISTS "${${python_include}}/Python.h")
      message(
        FATAL_ERROR
          " ${${python_include}}/Python.h does not exist, set proper ${python_include} to include directory"
      )
    endif()
  endif()
endmacro()

if(NOT LINK_AGAINST_PYTHON)
  # make sure provided python have Python.h header
  check_python_include(NRNPYTHON_INCLUDE3 3)
endif()
