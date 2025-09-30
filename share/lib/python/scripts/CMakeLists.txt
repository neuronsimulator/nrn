if(SKBUILD)
  set(scripts
      idraw
      mkthreadsafe
      modlunit
      neurondemo
      nmodl
      nocmodl
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
endif()
