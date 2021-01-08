# =============================================================================
# Common CXX and ISPC flags
# =============================================================================

# ISPC should compile with --pic by default
set(CMAKE_ISPC_FLAGS "${CMAKE_ISPC_FLAGS} --pic")

# =============================================================================
# NMODL CLI options : common and backend specific
# =============================================================================
# if user pass arguments then use those as common arguments
if ("${CORENRN_NMODL_FLAGS}" STREQUAL "")
  set(NMODL_COMMON_ARGS "passes --inline")
else()
  set(NMODL_COMMON_ARGS ${CORENRN_NMODL_FLAGS})
endif()

set(NMODL_CPU_BACKEND_ARGS "host --c")
set(NMODL_ISPC_BACKEND_ARGS "host --ispc")
set(NMODL_ACC_BACKEND_ARGS "host --c acc --oacc")

# =============================================================================
# Extract Compile definitions : common to all backend
# =============================================================================
get_directory_property(COMPILE_DEFS COMPILE_DEFINITIONS)
if(COMPILE_DEFS)
    set(CORENRN_COMMON_COMPILE_DEFS "")
    foreach(flag ${COMPILE_DEFS})
        set(CORENRN_COMMON_COMPILE_DEFS "${CORENRN_COMMON_COMPILE_DEFS} -D${flag}")
    endforeach()
endif()

# =============================================================================
# link flags : common to all backend
# =============================================================================
# ~~~
# find_cuda uses FindThreads that adds below imported target we
# shouldn't add imported target to link line
# ~~~
list(REMOVE_ITEM CORENRN_LINK_LIBS "Threads::Threads")

# replicate CMake magic to transform system libs to -l<libname>
foreach(link_lib ${CORENRN_LINK_LIBS})
    if(${link_lib} MATCHES "\-l.*")
        string(APPEND CORENRN_COMMON_LDFLAGS " ${link_lib}")
        continue()
    endif()
    get_filename_component(path ${link_lib} DIRECTORY)
    if(NOT path)
        string(APPEND CORENRN_COMMON_LDFLAGS " -l${link_lib}")
    elseif("${path}" MATCHES "^(/lib|/lib64|/usr/lib|/usr/lib64)$")
        get_filename_component(libname ${link_lib} NAME_WE)
        string(REGEX REPLACE "^lib" "" libname ${libname})
        string(APPEND CORENRN_COMMON_LDFLAGS " -l${libname}")
    else()
        string(APPEND CORENRN_COMMON_LDFLAGS " ${link_lib}")
    endif()
endforeach()

# =============================================================================
# compile flags : common to all backend
# =============================================================================
# PGI compiler adds --c++14;-A option for C++14, remove ";"
string(REPLACE ";" " " CXX14_STD_FLAGS "${CMAKE_CXX14_STANDARD_COMPILE_OPTION}")
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
set(CORENRN_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_BUILD_TYPE}} ${CXX14_STD_FLAGS}")

# =============================================================================
# nmodl/mod2c related options : TODO
# =============================================================================
# name of nmodl/mod2c binary
get_filename_component(nmodl_name ${CORENRN_MOD2CPP_BINARY} NAME)
set(nmodl_binary_name ${nmodl_name})
