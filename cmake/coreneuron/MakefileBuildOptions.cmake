# =============================================================================
# Copyright (C) 2016-2022 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# =============================================================================
# NMODL CLI options : common and backend specific
# =============================================================================
# ~~~
# if user pass arguments then use those as common arguments
# note that inlining is done by default
# ~~~
set(NMODL_COMMON_ARGS "passes --inline")

if(NOT "${CORENRN_NMODL_FLAGS}" STREQUAL "")
  string(APPEND NMODL_COMMON_ARGS " ${CORENRN_NMODL_FLAGS}")
endif()

set(NMODL_CPU_BACKEND_ARGS "host --c")
set(NMODL_ACC_BACKEND_ARGS "host --c acc --oacc")

# =============================================================================
# Construct the linker arguments that are used inside nrnivmodl-core (to build libcorenrnmech from
# libcoreneuron-core, libcoreneuron-cuda and mechanism object files) and inside nrnivmodl (to link
# NEURON's special against CoreNEURON's libcorenrnmech). These are stored in two global properties:
# CORENRN_LIB_LINK_FLAGS (used by NEURON/nrnivmodl to link special against CoreNEURON) and
# CORENRN_LIB_LINK_DEP_FLAGS (used by CoreNEURON/nrnivmodl-core to link libcorenrnmech.so).
# Conceptually: CORENRN_LIB_LINK_FLAGS = -lcorenrnmech $CORENRN_LIB_LINK_DEP_FLAGS
# =============================================================================
if(NOT CORENRN_ENABLE_SHARED)
  set_property(GLOBAL APPEND_STRING PROPERTY CORENRN_LIB_LINK_FLAGS " -Wl,--whole-archive")
endif()
set_property(GLOBAL APPEND_STRING PROPERTY CORENRN_LIB_LINK_FLAGS " -lcorenrnmech")
if(NOT CORENRN_ENABLE_SHARED)
  set_property(GLOBAL APPEND_STRING PROPERTY CORENRN_LIB_LINK_FLAGS " -Wl,--no-whole-archive")
endif()
# Essentially we "just" want to unpack the CMake dependencies of the `coreneuron-core` target into a
# plain string that we can bake into the Makefiles in both NEURON and CoreNEURON.
function(coreneuron_process_library_path library)
  get_filename_component(library_dir "${library}" DIRECTORY)
  if(NOT library_dir)
    # In case target is not a target but is just the name of a library, e.g. "dl"
    set_property(GLOBAL APPEND_STRING PROPERTY CORENRN_LIB_LINK_DEP_FLAGS " -l${library}")
  elseif("${library_dir}" MATCHES "^(/lib|/lib64|/usr/lib|/usr/lib64)$")
    # e.g. /usr/lib64/libpthread.so -> -lpthread TODO: consider using
    # https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_IMPLICIT_LINK_DIRECTORIES.html, or
    # dropping this special case entirely
    get_filename_component(libname ${library} NAME_WE)
    string(REGEX REPLACE "^lib" "" libname ${libname})
    set_property(GLOBAL APPEND_STRING PROPERTY CORENRN_LIB_LINK_DEP_FLAGS " -l${libname}")
  else()
    # It's a full path, include that on the line
    set_property(GLOBAL APPEND_STRING PROPERTY CORENRN_LIB_LINK_DEP_FLAGS
                                               " -Wl,-rpath,${library_dir} ${library}")
  endif()
endfunction()
function(coreneuron_process_target target)
  if(TARGET ${target})
    if(NOT target STREQUAL "coreneuron-core")
      # This is a special case: libcoreneuron-core.a is manually unpacked into .o files by the
      # nrnivmodl-core Makefile, so we do not want to also emit an -lcoreneuron-core argument.
      get_target_property(target_inc_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
      if(target_inc_dirs)
        foreach(inc_dir_genex ${target_inc_dirs})
          string(GENEX_STRIP "${inc_dir_genex}" inc_dir)
          if(inc_dir)
            set_property(GLOBAL APPEND_STRING PROPERTY CORENRN_EXTRA_COMPILE_FLAGS " -I${inc_dir}")
          endif()
        endforeach()
      endif()
      get_target_property(target_imported ${target} IMPORTED)
      if(target_imported)
        # In this case we can extract the full path to the library
        get_target_property(target_location ${target} LOCATION)
        coreneuron_process_library_path(${target_location})
      else()
        # This is probably another of our libraries, like -lcoreneuron-cuda. We might need to add -L
        # and an RPATH later.
        set_property(GLOBAL APPEND_STRING PROPERTY CORENRN_LIB_LINK_DEP_FLAGS " -l${target}")
      endif()
    endif()
    get_target_property(target_libraries ${target} LINK_LIBRARIES)
    if(target_libraries)
      foreach(child_target ${target_libraries})
        coreneuron_process_target(${child_target})
      endforeach()
    endif()
    return()
  endif()
  coreneuron_process_library_path("${target}")
endfunction()
coreneuron_process_target(coreneuron-core)
get_property(CORENRN_LIB_LINK_DEP_FLAGS GLOBAL PROPERTY CORENRN_LIB_LINK_DEP_FLAGS)
set_property(GLOBAL APPEND_STRING PROPERTY CORENRN_LIB_LINK_FLAGS " ${CORENRN_LIB_LINK_DEP_FLAGS}")
# In static builds then NEURON uses dlopen(nullptr, ...) to look for the corenrn_embedded_run
# symbol, which comes from libcoreneuron-core.a and gets included in libcorenrnmech.
if(NOT CORENRN_ENABLE_SHARED)
  set_property(GLOBAL APPEND_STRING PROPERTY CORENRN_LIB_LINK_FLAGS " -rdynamic")
endif()
get_property(CORENRN_EXTRA_COMPILE_FLAGS GLOBAL PROPERTY CORENRN_EXTRA_COMPILE_FLAGS)
get_property(CORENRN_LIB_LINK_FLAGS GLOBAL PROPERTY CORENRN_LIB_LINK_FLAGS)

# Detect if --start-group and --end-group are valid linker arguments. These are typically needed
# when linking mutually-dependent .o files (or where we don't know the correct order) on Linux, but
# they are not needed *or* recognised by the macOS linker.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
  include(CheckLinkerFlag)
  check_linker_flag(CXX -Wl,--start-group CORENRN_CXX_LINKER_SUPPORTS_START_GROUP)
elseif(CMAKE_SYSTEM_NAME MATCHES Linux)
  # Assume that --start-group and --end-group are only supported on Linux
  set(CORENRN_CXX_LINKER_SUPPORTS_START_GROUP ON)
endif()
if(CORENRN_CXX_LINKER_SUPPORTS_START_GROUP)
  set(CORENEURON_LINKER_START_GROUP -Wl,--start-group)
  set(CORENEURON_LINKER_END_GROUP -Wl,--end-group)
endif()

# Things that used to be in CORENRN_LIB_LINK_FLAGS: -lrt -L${CMAKE_HOST_SYSTEM_PROCESSOR}
# -L${caliper_LIB_DIR} -l${CALIPER_LIB}

# =============================================================================
# Turn CORENRN_COMPILE_DEFS into a list of -DFOO[=BAR] options.
# =============================================================================
list(TRANSFORM CORENRN_COMPILE_DEFS PREPEND -D OUTPUT_VARIABLE CORENRN_COMPILE_DEF_FLAGS)

# =============================================================================
# Extra link flags that we need to include when linking libcorenrnmech.{a,so} in CoreNEURON but that
# do not need to be passed to NEURON to use when linking nrniv/special (why?)
# =============================================================================
string(JOIN " " CORENRN_COMMON_LDFLAGS ${CORENRN_LIB_LINK_DEP_FLAGS} ${CORENRN_EXTRA_LINK_FLAGS})
string(JOIN " " CORENRN_SANITIZER_ENABLE_ENVIRONMENT_STRING ${NRN_SANITIZER_ENABLE_ENVIRONMENT})

# =============================================================================
# compile flags : common to all backend
# =============================================================================
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
string(
  JOIN
  " "
  CORENRN_CXX_FLAGS
  ${CMAKE_CXX_FLAGS}
  ${CMAKE_CXX_FLAGS_${_BUILD_TYPE}}
  ${CMAKE_CXX17_STANDARD_COMPILE_OPTION}
  ${NVHPC_ACC_COMP_FLAGS}
  ${NVHPC_CXX_INLINE_FLAGS}
  ${CORENRN_COMPILE_DEF_FLAGS}
  ${CORENRN_EXTRA_MECH_CXX_FLAGS}
  ${CORENRN_EXTRA_COMPILE_FLAGS})

# =============================================================================
# nmodl related options : TODO
# =============================================================================
# name of nmodl binary
get_filename_component(nmodl_name ${CORENRN_NMODL_BINARY} NAME)
set(nmodl_binary_name ${nmodl_name})
