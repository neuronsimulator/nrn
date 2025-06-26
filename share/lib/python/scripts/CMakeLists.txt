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
      sortspike)

  foreach(script IN LISTS scripts)
    install(
      PROGRAMS binwrapper.py
      DESTINATION "${SKBUILD_SCRIPTS_DIR}"
      RENAME "${script}")
  endforeach()
  # in order for mechanism building via CMake to work with a Python wheel, we need `binwrapper.py`
  install(PROGRAMS binwrapper.py DESTINATION "${NRN_INSTALL_DATA_PREFIX}share/nrn")
endif()
