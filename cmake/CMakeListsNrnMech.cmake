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
  get_filename_component(dir_path ${link_lib} DIRECTORY)
  if(NOT dir_path)
    string(APPEND NRN_LINK_DEFS " -l${link_lib}")
  elseif("${dir_path}" MATCHES "^(/lib|/lib64|/usr/lib|/usr/lib64)$")
    get_filename_component(libname ${link_lib} NAME_WE)
    string(REGEX REPLACE "^lib" "" libname ${libname})
    string(APPEND NRN_LINK_DEFS " -l${libname}")
  else()
    string(APPEND NRN_LINK_DEFS " ${link_lib}")
  endif()
endforeach()

# PGI add --c++11;-A option for c++11 flag
string(REPLACE ";" " " CXX11_STANDARD_COMPILE_OPTION "${CMAKE_CXX11_STANDARD_COMPILE_OPTION}")

# Compiler flags depending on cmake build type from BUILD_TYPE_<LANG>_FLAGS
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
set(BUILD_TYPE_C_FLAGS "${CMAKE_C_FLAGS_${_BUILD_TYPE}}")
set(BUILD_TYPE_CXX_FLAGS "${CMAKE_CXX_FLAGS_${_BUILD_TYPE}}")
