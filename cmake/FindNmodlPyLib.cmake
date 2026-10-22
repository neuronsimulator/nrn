# Set NRN_NMODL_PYLIB to the libpython that nmodl dlopens (even if we do not link it).
if(NRN_ENABLE_PYTHON
   AND NRN_LINK_AGAINST_PYTHON
   AND NRN_DEFAULT_PYTHON_LIBRARIES
   AND EXISTS "${NRN_DEFAULT_PYTHON_LIBRARIES}")
  set(NRN_NMODL_PYLIB "${NRN_DEFAULT_PYTHON_LIBRARIES}")
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
