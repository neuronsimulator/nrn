# =============================================================================
# Configure support for dynamic Python to use multiple Python versions
# =============================================================================
# ~~~
# NEURON can be built with python modules that can be usable from multiple
# versions of Python. Here we check if NRN_ENABLE_PYTHON_DYNAMIC is valid
# and determine an include directory for version 2 and/or 3 to build
# libnrnpython<major>.so. Depending on the pythons used, either or both of
# NRNPYTHON_INCLUDE3 or NRNPYTHON_INCLUDE2 will be defined.
#
# The above is good for mac and linux. Sadly, for MINGW, a distinct
# NRNPYTHON_INCLUDE<major><minor> is needed for each python in the
# NRN_PYTHON_DYNAMIC list. This is because libnrnpython<major><minor>.dll
# must be linked against the specfic libpython to avoid undefined name errors.
# Thus, at least for MINGW, parallel to the NRN_PYTHON_DYNAMIC list
# we construct the lists NRN_PYTHON_VER_LIST, NRN_PYTHON_INCLUDE_LIST,
# and NRN_PYTHON_LIB_LIST
#
# Note that if NRN_ENABLE_ABI3 is ON, then, regardless of the number of
# python 3 versions in the NRN_PYTHON_VER_LIST, only one hoc-abi3.so
# module will be built as that is compatible with all python >= 3.2.
# That is, all python version < 3.2 are built and have module names
# created by setup.py; for all python versions >=3,2, setup.py is called
# only once and the resulting module name created by setup.py is renamed
# to hoc-abi.so (linux/mac). To make these operations more clear, it is
# useful to make lists parallel to the NRN_PYTHON_EXE_LIST that
# specify whether pythonx.x should be executed and whether the
# module should be copied/(renamed?) according to the -abi3 suffix.
# ~~~

set(LINK_AGAINST_PYTHON ${MINGW})
set(NRN_PYTHON_EXE_LIST "" CACHE INTERNAL "" FORCE)
set(NRN_PYTHON_VER_LIST
    ""
    CACHE INTERNAL "" FORCE)
set(NRN_PYTHON_INCLUDE_LIST
    ""
    CACHE INTERNAL "" FORCE)
set(NRN_PYTHON_LIB_LIST
    ""
    CACHE INTERNAL "" FORCE)
set(NRN_PYTHON_CALL_SETUP_LIST "" CACHE INTERNAL "" FORCE)
set(NRN_PYTHON_MAKE_ABI3_LIST "" CACHE INTERNAL "" FORCE)

# ~~~
# Inform setup.py and nrniv/nrnpy.cpp whether libnrnpython name is libnrnpython<major>
# or libnrnpython<major><minor> . The latter is required for mingw.
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
      # NB: we are constructing here a variable name NRNPYTHON_INCLUDE${PYVER}
      set(NRNPYTHON_INCLUDE${PYVER} ${PYTHON_INCLUDE_DIRS})
      list(APPEND NRN_PYTHON_EXE_LIST ${PYTHON_EXECUTABLE})
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
            "from distutils.sysconfig import get_python_inc as p; print(p()); import sys; ${pr_pyver}; quit()"
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
          string(REGEX REPLACE [.] "" PYVER ${PYVER})
          list(APPEND NRN_PYTHON_EXE_LIST "${pyexe}")
          list(APPEND NRN_PYTHON_VER_LIST "${PYVER}")
          list(APPEND NRN_PYTHON_INCLUDE_LIST "${incval}")
          list(APPEND NRN_PYTHON_LIB_LIST "${PYTHON_LIBRARIES}")
        else()
          message(
            FATAL_ERROR "Error while checking ${pyexe} : ${result}\n${std_output}\n${err_output}")
        endif()
      endforeach()
    endif()

    # compute NRN_ENABLE_ABI3 association with NRN_PYTHON_EXE_LIST
    set(abi3_set 0)
    foreach(pyver ${NRN_PYTHON_VER_LIST})
      set(_setup 1)
      set(_mkabi 0)
      if (NRN_ENABLE_ABI3 AND pyver GREATER_EQUAL 3)
        if (NOT abi_set)
          set(abi_set 1)
          set(_setup 1)
          set(_mkabi 1)
        else()
          set(_setup 0)
        endif()
      else()
        set(_setup 1)
      endif()
      list(APPEND NRN_PYTHON_CALL_SETUP_LIST ${_setup})
      list(APPEND NRN_PYTHON_MAKE_ABI3_LIST ${_mkabi})
    endforeach()
    if (NRN_ENABLE_ABI3)
      message(STATUS "Relevant NRN_ENABLE_ABI3 info is...")
      message(STATUS "  NRN_PYTHON_EXE_LIST ${NRN_PYTHON_EXE_LIST}")
      message(STATUS "  NRN_PYTHON_VER_LIST ${NRN_PYTHON_VER_LIST}")
      message(STATUS "  NRN_PYTHON_CALL_SETUP_LIST ${NRN_PYTHON_CALL_SETUP_LIST}")
      message(STATUS "  NRN_PYTHON_MAKE_ABI3_LIST ${NRN_PYTHON_MAKE_ABI3_LIST}")
    endif()
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
  check_python_include(NRNPYTHON_INCLUDE2 2)
  check_python_include(NRNPYTHON_INCLUDE3 3)
endif()
