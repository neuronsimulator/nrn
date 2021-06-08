# =============================================================================
# Prepare nrnivmodl script with correct flags
# =============================================================================

# extract the COMPILE_DEFINITIONS property from the directory
get_directory_property(NRN_COMPILE_DEFS COMPILE_DEFINITIONS)
if(NRN_COMPILE_DEFS)
  set(NRN_COMPILE_DEFS "")
  foreach(flag ${NRN_COMPILE_DEFS})
    set(NRN_COMPILE_DEFS "${NRN_COMPILE_DEFS} -D${flag}")
  endforeach()
endif()

# extract link defs to the whole project
get_target_property(NRN_LINK_LIBS nrniv_lib LINK_LIBRARIES)
if(NOT NRN_LINK_LIBS)
  set(NRN_LINK_LIBS "")
endif()

# Interview might have linked to libnrniv but we don't want to link to special
list(REMOVE_ITEM NRN_LINK_LIBS "interviews")

# CMake does some magic to transform sys libs to -l<libname>. We replicate it
foreach(link_lib ${NRN_LINK_LIBS})
  # skip static readline library as it will be linked to nrniv (e.g. with wheel)
  # also stub libraries from OSX can be skipped
  if ("${link_lib}" MATCHES "(libreadline.a|/*.tbd)")
    continue()
  endif()

  get_filename_component(dir_path ${link_lib} DIRECTORY)
  if(TARGET ${link_lib})
    message(NOTICE "Using Target in compiling and linking, you should take care of it if
                    it fail miserabily (CODE: 1234567890)")
    get_property(link_flag TARGET ${link_lib} PROPERTY INTERFACE_LINK_LIBRARIES)
    string(APPEND NRN_LINK_DEFS ${link_flag})
    # Not use it yet because it can be generator expressions
    # get_property(compile_flag TARGET ${link_lib} PROPERTY INTERFACE_COMPILE_OPTIONS)
    # string(APPEND NRN_COMPILE_DEFS ${compile_flag})
    continue()
  elseif(NOT dir_path)
    string(APPEND NRN_LINK_DEFS " -l${link_lib}")
  # avoid library paths from special directory /nrnwheel which
  # used to build wheels under docker container
  elseif("${dir_path}" MATCHES "^/nrnwheel")
    continue()
  elseif("${dir_path}" MATCHES "^(/lib|/lib64|/usr/lib|/usr/lib64)$")
    # NAME_WLE not avaialble with CMake version < 3.14
    get_filename_component(libname ${link_lib} NAME)
    string(REGEX REPLACE "\\.[^.]*$" "" libname_wle ${libname})
    string(REGEX REPLACE "^lib" "" libname_wle ${libname_wle})
    string(APPEND NRN_LINK_DEFS " -l${libname_wle}")
  else()
    string(APPEND NRN_LINK_DEFS " ${link_lib} -Wl,-rpath,${dir_path}")
  endif()
endforeach()

# PGI add --c++11;-A option for c++11 flag
string(REPLACE ";" " " CXX11_STANDARD_COMPILE_OPTION "${CMAKE_CXX11_STANDARD_COMPILE_OPTION}")

# Compiler flags depending on cmake build type from BUILD_TYPE_<LANG>_FLAGS
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
set(BUILD_TYPE_C_FLAGS "${CMAKE_C_FLAGS_${_BUILD_TYPE}}")
set(BUILD_TYPE_CXX_FLAGS "${CMAKE_CXX_FLAGS_${_BUILD_TYPE}}")
