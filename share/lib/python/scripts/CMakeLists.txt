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
      PROGRAMS _binwrapper.py
      DESTINATION "${SKBUILD_SCRIPTS_DIR}"
      RENAME "${script}")
  endforeach()
endif()
