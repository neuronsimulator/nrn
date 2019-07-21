# =============================================================================
# Configure support for dynamic Python to use multiple Python versions
# =============================================================================
# NEURON can be built with python module that can be usabe from muliple
# versions of Python. Here we check if NRN_ENABLE_PYTHON_DYNAMIC is valid
# and determine an include directory for version 2 and/or 3 to build
# libnrnpython<major>.so. Depending on the pythons used either or both of
# NRNPYTHON_INCLUDE3 or NRNPYTHON_INCLUDE2 will be defined.

if (NRN_ENABLE_PYTHON_DYNAMIC MATCHES "NO" OR NOT NRN_ENABLE_PYTHON)
  # nothing to do
  message(STATUS "Python dynamic support disabled")
elseif (NRN_ENABLE_PYTHON_DYNAMIC MATCHES "YES")
  # use the default python already determined
  set (NRNPYTHON_INCLUDE${PYTHON_VERSION_MAJOR} ${PYTHON_INCLUDE_DIRS})
  message(STATUS "Python dynamic support with headers: ${NRNPYTHON_INCLUDE${PYTHON_VERSION_MAJOR}}")
else()
  # run each python provided by user to determine major and include directory
  message(STATUS "Dynamic Python support")
  foreach(pyexe ${NRN_ENABLE_PYTHON_DYNAMIC})
    message(STATUS "  checking if ${pyexe} is a working python")
    execute_process(COMMAND ${pyexe} -c "from distutils.sysconfig import get_python_inc as p ; print(p()) ; import sys ; print(sys.version_info[0]) ; quit()"
      RESULT_VARIABLE result
      OUTPUT_VARIABLE std_output
      ERROR_VARIABLE  err_output
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(result EQUAL 0)
      string(REGEX MATCH .$ major_version ${std_output})
      string(REGEX MATCH ^[^\n]* incval ${std_output})
      if (NOT NRNPYTHON_INCLUDE${major_version})
        set (NRNPYTHON_INCLUDE${major_version} ${incval})
      endif()
    else()
      message(FATAL_ERROR "Error while checking ${pyexe} : ${result}")
    endif()
  endforeach()
endif()

# check Python.h exists under provided include directory
macro(check_python_include python_include major_version)
  if (${python_include})
    if (NOT EXISTS "${${python_include}}/Python.h")
      message(FATAL_ERROR " ${${python_include}}/Python.h does not exist, set proper ${python_include} to include directory")
    endif()
  endif()
endmacro()

# make sure provided python have Python.h header
check_python_include(NRNPYTHON_INCLUDE2 2)
check_python_include(NRNPYTHON_INCLUDE3 3)
