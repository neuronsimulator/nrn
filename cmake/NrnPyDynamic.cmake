# verify that NRN_ENABLE_PYTHON_DYNAMIC is valid and determine
# an include directory for version 2 and/or 3 for
# building libnrnpython<major>.so
# Depending on the pythons used either or both of
# NRNPYTHON_INCLUDE3 or NRNPYTHON_INCLUDE2 will be defined

if (NRN_ENABLE_PYTHON_DYNAMIC MATCHES "NO" OR NOT NRN_ENABLE_PYTHON)
  #do nothing
elseif (NRN_ENABLE_PYTHON_DYNAMIC MATCHES "YES")
  #use the default python already determined
  set (NRNPYTHON_INCLUDE${PYTHON_VERSION_MAJOR} ${PYTHON_INCLUDE_DIRS})
else()
  #run each python to determine major and include dir.
  #fatal error if any fails
  foreach(pyexe ${NRN_ENABLE_PYTHON_DYNAMIC})
    message(STATUS "checking that ${pyexe} is a working python")
    # worked on my machine with python3x but python2 gave wrong inc dir
    #execute_process(COMMAND ${pyexe} -c "from sysconfig import get_paths as p ; print(p()[\"include\"]) ; import sys ; print(sys.version_info[0]) ; quit()"
    execute_process(COMMAND ${pyexe} -c "from distutils.sysconfig import get_python_inc as p ; print(p()) ; import sys ; print(sys.version_info[0]) ; quit()"
      RESULT_VARIABLE res
      OUTPUT_VARIABLE outstd
      ERROR_VARIABLE  errstd
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (res EQUAL 0)
      string(REGEX MATCH .$ majorver ${outstd})
      string(REGEX MATCH ^[^\n]* incval ${outstd})
      if (NOT NRNPYTHON_INCLUDE${majorver})
        set (NRNPYTHON_INCLUDE${majorver} ${incval})
      endif()
    else()
      message(FATAL_ERROR " ${pyexe} : ${res} : ${errstd}")
    endif()
  endforeach()
endif()

# a final check that Python.h exists

macro(chk_pyh pyinc majorver)
  if (${pyinc})
    if (NOT EXISTS "${${pyinc}}/Python.h")
      message(FATAL_ERROR " ${${pyinc}}/Python.h does not exist: Set ${pyinc} to the proper python${majorver} include directory")
    endif()
  endif()
endmacro()

chk_pyh(NRNPYTHON_INCLUDE2 2)
chk_pyh(NRNPYTHON_INCLUDE3 3)

#message(NOTICE " NRNPYTHON_INCLUDE2 ${NRNPYTHON_INCLUDE2}")
#message(NOTICE " NRNPYTHON_INCLUDE3 ${NRNPYTHON_INCLUDE3}")

