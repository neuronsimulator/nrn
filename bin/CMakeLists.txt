# =============================================================================
# Copyright (C) 2016-2020 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# =============================================================================
# Prepare nrnivmodl-core script with correct compiler flags
# =============================================================================

# extract the COMPILE_DEFINITIONS property from the directory.
get_directory_property(CORENRN_COMPILE_FLAGS COMPILE_DEFINITIONS)
if(CORENRN_COMPILE_FLAGS)
  set(CORENRN_COMPILE_DEFS "")
  foreach(flag ${CORENRN_COMPILE_FLAGS})
    set(CORENRN_COMPILE_DEFS "${CORENRN_COMPILE_DEFS} -D${flag}")
  endforeach()
endif()

# ~~~
# find_cuda uses FindThreads that adds below imported target we shouldn't add
# imported target to link line
# ~~~
list(REMOVE_ITEM CORENRN_LINK_LIBS "Threads::Threads")
# replicate CMake magic to transform system libs to -l<libname>
foreach(link_lib ${CORENRN_LINK_LIBS})
  if(${link_lib} MATCHES "\-l.*")
      string(APPEND CORENRN_LINK_DEFS " ${link_lib}")
      continue()
  endif()
  get_filename_component(path ${link_lib} DIRECTORY)
  if(NOT path)
    string(APPEND CORENRN_LINK_DEFS " -l${link_lib}")
  elseif("${path}" MATCHES "^(/lib|/lib64|/usr/lib|/usr/lib64)$")
    get_filename_component(libname ${link_lib} NAME_WE)
    string(REGEX REPLACE "^lib" "" libname ${libname})
    string(APPEND CORENRN_LINK_DEFS " -l${libname}")
  else()
    string(APPEND CORENRN_LINK_DEFS " ${link_lib}")
  endif()
endforeach()

# PGI compiler add --c++14;-A option for c++14 flag
string(REPLACE ";" " " CXX14_STANDARD_COMPILE_OPTION "${CMAKE_CXX14_STANDARD_COMPILE_OPTION}")

# compiler flags depending on BUILD_TYPE (configured as BUILD_TYPE_<LANG>_FLAGS)
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
set(BUILD_TYPE_C_FLAGS "${CMAKE_C_FLAGS_${_BUILD_TYPE}}")
set(BUILD_TYPE_CXX_FLAGS "${CMAKE_CXX_FLAGS_${_BUILD_TYPE}}")
message(STATUS "CXX Compile flags from BUILD_TYPE (${_BUILD_TYPE}): ${BUILD_TYPE_CXX_FLAGS}")

# nmodl options
if(CORENRN_ENABLE_NMODL)
  set(nmodl_arguments_c "host --c passes --inline ${CORENRN_NMODL_FLAGS}")
  set(nmodl_arguments_ispc "host --ispc passes --inline ${CORENRN_NMODL_FLAGS}")
else()
  set(nmodl_arguments_c "")
  set(nmodl_arguments_ispc "")
endif()

# name of nmodl/mod2c binary
get_filename_component(nmodl_name ${CORENRN_MOD2CPP_BINARY} NAME)
set(nmodl_binary_name ${nmodl_name})

# =============================================================================
# Install first into build directory and then to install prefix
# =============================================================================

configure_file(nrnivmodl_core_makefile.in
               ${CMAKE_BINARY_DIR}/share/coreneuron/nrnivmodl_core_makefile @ONLY)
configure_file(nrnivmodl-core.in ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/share/coreneuron/nrnivmodl_core_makefile
        DESTINATION share/coreneuron)
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/nrnivmodl-core DESTINATION bin)
