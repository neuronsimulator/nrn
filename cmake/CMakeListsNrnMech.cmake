# =============================================================================
# Prepare nrnivmodl script with correct flags
# =============================================================================

# extract the COMPILE_DEFINITIONS property from the directory
get_directory_property(NRN_COMPILE_DEFS_DIR_PROPERTY COMPILE_DEFINITIONS)
list(APPEND NRN_COMPILE_DEFS ${NRN_COMPILE_DEFS_DIR_PROPERTY})

# extract link defs to the whole project
get_target_property(NRN_LINK_LIBS nrniv_lib LINK_LIBRARIES)
if(NOT NRN_LINK_LIBS)
  set(NRN_LINK_LIBS "")
endif()

# Interview might have linked to libnrniv but we don't want to link to special
list(REMOVE_ITEM NRN_LINK_LIBS "interviews")

# CMake does some magic to transform sys libs to -l<libname>. We replicate it
foreach(link_lib ${NRN_LINK_LIBS})
  # skip static readline library as it will be linked to nrniv (e.g. with wheel) also stub libraries
  # from OSX can be skipped
  if("${link_lib}" MATCHES "(libreadline.a|/*.tbd)")
    continue()
  endif()

  get_filename_component(dir_path ${link_lib} DIRECTORY)
  if(TARGET ${link_lib})
    get_property(
      link_flag
      TARGET ${link_lib}
      PROPERTY INTERFACE_LINK_LIBRARIES)
    set(description
        "Extracting link flags from target '${link_lib}', beware that this can be fragile.")
    # Not use it yet because it can be generator expressions
    get_property(
      compile_flag
      TARGET ${link_lib}
      PROPERTY INTERFACE_COMPILE_OPTIONS)
    string(GENEX_STRIP "${compile_flag}" compile_flag)
    list(APPEND NRN_COMPILE_FLAGS "${compile_flag}")
    get_property(
      include_dirs
      TARGET ${link_lib}
      PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    # TODO support generator expressions. I think this can be done with some combination of
    # file(GENERATE OUTPUT ...) to get the build-time values of the expressions and install(EXPORT
    # ...) to get the installed values?
    foreach(include_dir_genex ${include_dirs})
      string(GENEX_STRIP "${include_dir_genex}" include_dir)
      if(include_dir)
        list(APPEND NRN_COMPILE_FLAGS "-I${include_dir}")
      endif()
    endforeach()
  elseif(NOT dir_path)
    set(link_flag "-l${link_lib}")
    set(description
        "Generating link flags from name '${link_lib}', beware that this can be fragile.")
    # avoid library paths from special directory /nrnwheel which used to build wheels under docker
    # container
  elseif("${dir_path}" MATCHES "^/nrnwheel")
    continue()
  elseif("${dir_path}" MATCHES "^(/lib|/lib64|/usr/lib|/usr/lib64)$")
    # NAME_WLE not avaialble with CMake version < 3.14
    get_filename_component(libname ${link_lib} NAME)
    string(REGEX REPLACE "\\.[^.]*$" "" libname_wle ${libname})
    string(REGEX REPLACE "^lib" "" libname_wle ${libname_wle})
    set(link_flag "-l${libname_wle}")
    set(description
        "Extracting link flags from path '${link_lib}', beware that this can be fragile.")
  else()
    set(link_flag "${link_lib} -Wl,-rpath,${dir_path}")
    set(description "Generating link flags from path ${link_lib}")
  endif()
  message(NOTICE "${description} Got: ${link_flag}")
  string(APPEND NRN_LINK_DEFS " ${link_flag}")
endforeach()

# Turn the CMake lists NRN_COMPILE_DEFS, NRN_COMPILE_FLAGS, NRN_LINK_FLAGS and
# NRN_NOCMODL_SANITIZER_ENVIRONMENT into flat strings
list(TRANSFORM NRN_COMPILE_DEFS PREPEND -D OUTPUT_VARIABLE NRN_COMPILE_DEF_FLAGS)
string(JOIN " " NRN_COMPILE_DEFS_STRING ${NRN_COMPILE_DEF_FLAGS})
string(JOIN " " NRN_COMPILE_FLAGS_STRING ${NRN_COMPILE_FLAGS} ${NRN_EXTRA_MECH_CXX_FLAGS})
string(JOIN " " NRN_LINK_FLAGS_STRING ${NRN_LINK_FLAGS} ${NRN_LINK_FLAGS_FOR_ENTRY_POINTS})
string(JOIN " " NRN_NOCMODL_SANITIZER_ENVIRONMENT_STRING ${NRN_NOCMODL_SANITIZER_ENVIRONMENT})

# Compiler flags depending on cmake build type from BUILD_TYPE_<LANG>_FLAGS
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
set(BUILD_TYPE_C_FLAGS "${CMAKE_C_FLAGS_${_BUILD_TYPE}}")
set(BUILD_TYPE_CXX_FLAGS "${CMAKE_CXX_FLAGS_${_BUILD_TYPE}}")
