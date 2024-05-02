# =============================================================================
# Prepare nrnivmodl script with correct flags
# =============================================================================

# extract the COMPILE_DEFINITIONS property from the directory
get_directory_property(NRN_COMPILE_DEFS_DIR_PROPERTY COMPILE_DEFINITIONS)
list(APPEND NRN_COMPILE_DEFS ${NRN_COMPILE_DEFS_DIR_PROPERTY})

# Turn the CMake lists NRN_COMPILE_DEFS, NRN_COMPILE_FLAGS, NRN_LINK_FLAGS and
# NRN_NOCMODL_SANITIZER_ENVIRONMENT into flat strings
list(TRANSFORM NRN_COMPILE_DEFS PREPEND -D OUTPUT_VARIABLE NRN_COMPILE_DEF_FLAGS)
string(JOIN " " NRN_COMPILE_DEFS_STRING ${NRN_COMPILE_DEF_FLAGS})
string(JOIN " " NRN_COMPILE_FLAGS_STRING ${NRN_COMPILE_FLAGS} ${NRN_EXTRA_MECH_CXX_FLAGS})
string(JOIN " " NRN_LINK_FLAGS_STRING ${NRN_LINK_FLAGS} ${NRN_LINK_FLAGS_FOR_ENTRY_POINTS})
string(JOIN " " NRN_NOCMODL_SANITIZER_ENVIRONMENT_STRING ${NRN_NOCMODL_SANITIZER_ENVIRONMENT})

get_link_libraries(NRN_LINK_DEFS NOT_USE_INCLUDE_HERE nrniv_lib interviews)

# Compiler flags depending on cmake build type from BUILD_TYPE_<LANG>_FLAGS
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
set(BUILD_TYPE_C_FLAGS "${CMAKE_C_FLAGS_${_BUILD_TYPE}}")
set(BUILD_TYPE_CXX_FLAGS "${CMAKE_CXX_FLAGS_${_BUILD_TYPE}}")
