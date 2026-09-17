# Set NRN_NMODL_PYLIB to the libpython that nmodl dlopens (even if we do not link it).
if(NRN_ENABLE_PYTHON
   AND NRN_LINK_AGAINST_PYTHON
   AND NRN_DEFAULT_PYTHON_LIBRARIES)
  set(NRN_NMODL_PYLIB "${NRN_DEFAULT_PYTHON_LIBRARIES}")
elseif(NRN_DEFAULT_PYTHON_EXECUTABLE)
  execute_process(
    COMMAND
      "${NRN_DEFAULT_PYTHON_EXECUTABLE}" -c
      "import os, sysconfig; print(os.path.join(sysconfig.get_config_var('LIBDIR') or '', sysconfig.get_config_var('LDLIBRARY') or ''))"
    RESULT_VARIABLE _nmodl_pylib_rc
    OUTPUT_VARIABLE NRN_NMODL_PYLIB
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE _nmodl_pylib_err)
  if(_nmodl_pylib_rc OR NRN_NMODL_PYLIB STREQUAL "")
    message(
      WARNING
        "Could not discover libpython for NMODL_PYLIB (rc=${_nmodl_pylib_rc}): ${_nmodl_pylib_err}")
    set(NRN_NMODL_PYLIB "")
  endif()
endif()
if(NRN_NMODL_PYLIB)
  message(STATUS "NMODL_PYLIB    | ${NRN_NMODL_PYLIB}")
endif()
