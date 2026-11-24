# Set NRN_NMODL_PYLIB to the libpython that nmodl dlopens (even if we do not link it).
# Python_LIBRARIES is that shared object on Unix. On Windows it is an import library (.lib or
# .dll.a); LoadLibrary needs Python_RUNTIME_LIBRARY, the DLL.
set(_nmodl_dlopen_lib "")
if(NRN_ENABLE_PYTHON AND NRN_LINK_AGAINST_PYTHON)
  if(EXISTS "${NRN_DEFAULT_PYTHON_RUNTIME_LIBRARY}")
    set(_nmodl_dlopen_lib "${NRN_DEFAULT_PYTHON_RUNTIME_LIBRARY}")
  elseif(EXISTS "${NRN_DEFAULT_PYTHON_LIBRARIES}" AND NOT NRN_DEFAULT_PYTHON_LIBRARIES MATCHES
                                                      "\\.(lib|a)$")
    set(_nmodl_dlopen_lib "${NRN_DEFAULT_PYTHON_LIBRARIES}")
  endif()
endif()
if(_nmodl_dlopen_lib)
  set(NRN_NMODL_PYLIB "${_nmodl_dlopen_lib}")
elseif(NRN_DEFAULT_PYTHON_EXECUTABLE)
  execute_process(
    COMMAND "${NRN_DEFAULT_PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/find_nmodl_pylib.py"
    RESULT_VARIABLE _nmodl_pylib_rc
    OUTPUT_VARIABLE NRN_NMODL_PYLIB
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE _nmodl_pylib_err)
  if(_nmodl_pylib_rc
     OR NRN_NMODL_PYLIB STREQUAL ""
     OR NOT EXISTS "${NRN_NMODL_PYLIB}")
    message(
      WARNING
        "Could not discover libpython for NMODL_PYLIB (rc=${_nmodl_pylib_rc} path='${NRN_NMODL_PYLIB}'): ${_nmodl_pylib_err}"
    )
    set(NRN_NMODL_PYLIB "")
  endif()
endif()
if(NRN_NMODL_PYLIB)
  message(STATUS "NMODL_PYLIB    | ${NRN_NMODL_PYLIB}")
endif()
