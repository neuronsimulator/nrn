if(SKBUILD)
  set(scripts
      idraw
      mkthreadsafe
      modlunit
      neurondemo
      nmodl
      nrngui
      nrniv
      nrniv-core
      nrnivmodl
      nrnivmodl-core
      nrnpyenv.sh
      sortspike
      rdcellstate
      nrnivmodl-cmake)

  foreach(script IN LISTS scripts)
    install(
      PROGRAMS binwrapper.py
      DESTINATION "${SKBUILD_SCRIPTS_DIR}"
      RENAME "${script}")
    if(WIN32)
      # cmd.exe searches PATHEXT (.CMD), not extensionless Python files.
      install(
        PROGRAMS binwrapper.cmd
        DESTINATION "${SKBUILD_SCRIPTS_DIR}"
        RENAME "${script}.cmd")
    endif()
  endforeach()
  if(NRN_ENABLE_CORENEURON)
    install(
      PROGRAMS binwrapper.py
      DESTINATION "${SKBUILD_SCRIPTS_DIR}"
      RENAME "nrnivmodl-all-cmake")
    if(WIN32)
      install(
        PROGRAMS binwrapper.cmd
        DESTINATION "${SKBUILD_SCRIPTS_DIR}"
        RENAME "nrnivmodl-all-cmake.cmd")
    endif()
  endif()
  # in order for mechanism building via CMake to work with a Python wheel, we need `binwrapper.py`
  install(PROGRAMS binwrapper.py DESTINATION "${NRN_INSTALL_DATA_PREFIX}share/nrn")
endif()
