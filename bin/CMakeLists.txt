set(nmodl_makefile "nrnivmodl_core_makefile")
set(nmodl_compile_scipt "nrnivmodl-core")

get_directory_property(CORENRN_COMPILE_FLAGS COMPILE_DEFINITIONS)

if (CORENRN_COMPILE_FLAGS)
    set(CORENRN_COMPILE_DEFS "-D${CORENRN_COMPILE_FLAGS}")
    string(REPLACE ";" " -D" CORENRN_COMPILE_DEFS "${CORENRN_COMPILE_DEFS}")
endif()

# Link libraries
# CMake does some magic to transform sys libs to -l<libname>. We replicate it
foreach(link_lib ${CORENRN_LINK_LIBS})
    get_filename_component(pth ${link_lib} DIRECTORY)
    if(NOT pth)
       string(APPEND CORENRN_LINK_DEFS " -l${link_lib}")
    elseif("${pth}" MATCHES "^(/lib|/lib64|/usr/lib|/usr/lib64)$")
        get_filename_component(libname ${link_lib} NAME_WE)
        string(REGEX REPLACE "^lib" "" libname ${libname})
        string(APPEND CORENRN_LINK_DEFS " -l${libname}")
    else()
        string(APPEND CORENRN_LINK_DEFS " ${link_lib}")
    endif()
endforeach()
#message("CORENRN_LINK_LIBS: ${CORENRN_LINK_LIBS}, CORENRN_LINK_DEFS: ${CORENRN_LINK_DEFS}")

# Additional mods
string(REPLACE ";" " " CORENRN_OPTMODS "${CORENRN_OPTMECH_MODS}")
string(REPLACE ";" " " CORENRN_OPTMODS_OUTPUTS "${CORENRN_OPTMECH_OUTPUTS}")

# PGI add --c++11;-A option for c++11 flag
string(REPLACE ";" " " CXX11_STANDARD_COMPILE_OPTION "${CMAKE_CXX11_STANDARD_COMPILE_OPTION}")

# MPI path might be multiple directories
if (MPI_C_INCLUDE_PATH)
    string(REPLACE ";" " -I" CORENRN_MPI_INCLUDES "${MPI_C_INCLUDE_PATH}")
endif()

# Compiler flags depending on BUILD_TYPE shared as BUILD_TYPE_<LANG>_FLAGS
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
set(BUILD_TYPE_C_FLAGS "${CMAKE_C_FLAGS_${_BUILD_TYPE}}")
set(BUILD_TYPE_CXX_FLAGS "${CMAKE_CXX_FLAGS_${_BUILD_TYPE}}")
message("CXX Compile Flags from BUILD_TYPE: ${BUILD_TYPE_CXX_FLAGS}")

if(ENABLE_NMODL)
    set(nmodl_arguments_c "host --c ${NMODL_EXTRA_FLAGS}")
    set(nmodl_arguments_ispc "host --ispc ${NMODL_EXTRA_FLAGS}")
    set(mod2c_install_bin "${nmodl_BINARY}")
else ()
    set(nmodl_arguments_c "")
    set(nmodl_arguments_ispc "")
    set(mod2c_install_bin "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/mod2c_core")
endif ()

configure_file(${nmodl_makefile}.in ${nmodl_makefile} @ONLY)

# Makefile
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${nmodl_makefile}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/coreneuron/)

# Launcher
install(PROGRAMS ${nmodl_compile_scipt}
        DESTINATION ${CMAKE_INSTALL_BINDIR})
