# =============================================================================
# Build nrniv binary and corresponding library
# =============================================================================

# ~~~
# The old build (autoconf) built executables in oc, nrnoc, ivoc, nrniv as
# well as libraries there and in a dozen or so other places. For cmake we
# are experimenting with a single libnrniv.so that combines many of files
# in those folders, both for simplicity, and to avoid issues of dependency
# among the old libraries.
# Also the oc, nrnoc, and ivoc executables are pretty well useless these
# days. Other groups can, and should be, factored out.
# ~~~

# =============================================================================
# Build modlunit : Mod file units checker
# =============================================================================
flex_target(modlunitlexer ${NRN_MODLUNIT_SRC_DIR}/lex.l ${NRN_MODLUNIT_SRC_DIR}/lex.c)

bison_target(modlunitparser ${NRN_MODLUNIT_SRC_DIR}/parse1.y ${NRN_MODLUNIT_SRC_DIR}/parse1.c
             DEFINES_FILE ${NRN_MODLUNIT_SRC_DIR}/parse1.h)

set_property(
  SOURCE ${NRN_MODLUNIT_SRC_FILES}
  APPEND
  PROPERTY COMPILE_DEFINITIONS NRNUNIT=1)

add_executable(modlunit ${NRN_MODLUNIT_SRC_FILES})

# =============================================================================
# Build nocmodl : source-to-source compiler for NMODL
# =============================================================================
flex_target(nocmodllexer ${NRN_NMODL_SRC_DIR}/lex.l ${NRN_NMODL_SRC_DIR}/lex.c)

bison_target(nocmodlparser ${NRN_NMODL_SRC_DIR}/parse1.y ${NRN_NMODL_SRC_DIR}/parse1.c DEFINES_FILE
             ${NRN_NMODL_SRC_DIR}/parse1.h)

bison_target(nocmodlparser ${NRN_NMODL_SRC_DIR}/diffeq.y ${NRN_NMODL_SRC_DIR}/diffeq.c DEFINES_FILE
             ${NRN_NMODL_SRC_DIR}/diffeq.h)

set_property(
  SOURCE ${NRN_NMODL_SRC_FILES}
  APPEND
  PROPERTY COMPILE_DEFINITIONS NMODL=1 NOCMODL=1 CVODE=1 VECTORIZE=1)

add_executable(nocmodl ${NRN_NMODL_SRC_FILES})

# =============================================================================
# Translate all MOD files to C and mark them generated
# =============================================================================
foreach(modfile ${NRN_MODFILE_BASE_NAMES})
  nocmodl_mod_to_c(${modfile})
  list(APPEND NRN_MODFILE_C ${modfile}.c)
endforeach()

set_source_files_properties(${NRN_MODFILE_C} PROPERTIES GENERATED TRUE)

# =============================================================================
# Bison parser for HOC interpreter
# =============================================================================
bison_target(ocparser ${NRN_OC_SRC_DIR}/parse.y ${NRN_OC_SRC_DIR}/parse.c DEFINES_FILE
             ${NRN_OC_SRC_DIR}/parse.h)

# =============================================================================
# Source code lists
# =============================================================================
set(NRN_BASE_SRC_FILES ${NRN_IVOC_SRC_FILES} ${NRN_NRNIV_SRC_FILES} ${NRN_NRNOC_SRC_FILES}
                       ${NRN_OC_SRC_FILES})

set(NRN_NRNIV_LIB_SRC_FILES
    ${NRN_BASE_SRC_FILES}
    ${NRN_MEMACS_SRC_FILES}
    ${NRN_MESCH_SRC_FILES}
    ${NRN_MODFILE_C}
    ${NRN_NRNGNU_SRC_FILES}
    ${NRN_SCOPMATH_SRC_FILES}
    ${NRN_SPARSE13_SRC_FILES}
    ${NRN_SUNDIALS_SRC_FILES})

if(NRN_ENABLE_MPI)
  list(APPEND NRN_NRNIV_LIB_SRC_FILES ${NRN_PARALLEL_SRC_FILES})
endif()

if(NRN_ENABLE_MPI_DYNAMIC)
  list(APPEND NRN_NRNIV_LIB_SRC_FILES ${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpi_dynam.c)
else()
  list(APPEND NRN_NRNIV_LIB_SRC_FILES ${NRN_NRNMPI_SRC_FILES})
endif()

# NRN_ENABLE_MODULE_INSTALL will create a separate nrnpython lib
if(NRN_ENABLE_PYTHON AND NOT NRN_ENABLE_PYTHON_DYNAMIC)
  # Include nrnpython in nrniv - useful for single lib neuron and wheels
  list(APPEND NRN_NRNIV_LIB_SRC_FILES ${NRN_NRNPYTHON_SRC_FILES})
endif()

if(NOT NRN_ENABLE_INTERVIEWS)
  list(APPEND NRN_NRNIV_LIB_SRC_FILES ${NRN_IVOS_SRC_FILES})
endif()

# =============================================================================
# Include directories for #include <../../nrnconf.h>
# =============================================================================
set(NRN_INCLUDE_DIRS
    ${NRN_IVOC_SRC_DIR}
    ${NRN_NRNIV_SRC_DIR}
    ${NRN_NRNOC_SRC_DIR}
    ${NRN_OC_SRC_DIR}
    ${PROJECT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/src/nrncvode
    ${PROJECT_BINARY_DIR}/src/nrnjava
    ${PROJECT_BINARY_DIR}/src/nrnoc
    ${PROJECT_BINARY_DIR}/src/nrnpython
    ${PROJECT_BINARY_DIR}/src/oc
    ${PROJECT_BINARY_DIR}/src/parallel
    ${PROJECT_BINARY_DIR}/src/sundials
    ${PROJECT_BINARY_DIR}/src/sundials/shared
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/gnu
    ${PROJECT_SOURCE_DIR}/src/memacs
    ${PROJECT_SOURCE_DIR}/src/mesch
    ${PROJECT_SOURCE_DIR}/src/nrncvode
    ${PROJECT_SOURCE_DIR}/src/nrnmpi
    ${PROJECT_SOURCE_DIR}/src/nrnpython
    ${PROJECT_SOURCE_DIR}/src/parallel
    ${PROJECT_SOURCE_DIR}/src/sparse
    ${PROJECT_SOURCE_DIR}/src/sparse13
    ${PROJECT_SOURCE_DIR}/src/sundials
    ${PROJECT_SOURCE_DIR}/src/sundials/cvodes
    ${PROJECT_SOURCE_DIR}/src/sundials/ida
    ${PROJECT_SOURCE_DIR}/src/sundials/shared)

# =============================================================================
# Helper commands : generate various headers
# =============================================================================
# ~~~
# generate version information file
# nrnversion.h does not depend on another file but on the output of
# git2version_h.sh and nrnversion.h should only be changed if that output
# is different from the contents of nrnversion.h
# ~~~
add_custom_target(
  nrnversion_h ${PROJECT_SOURCE_DIR}/git2nrnversion_h.sh ${PROJECT_SOURCE_DIR} > nrnversion.h.tmp
  COMMAND ${CMAKE_COMMAND} -E copy_if_different nrnversion.h.tmp ${NRN_NRNOC_SRC_DIR}/nrnversion.h
  DEPENDS ${PROJECT_SOURCE_DIR}/git2nrnversion_h.sh)

add_custom_command(OUTPUT ${NRN_NRNOC_SRC_DIR}/nrnversion.h DEPENDS nrnversion_h)

# avoid error with nvector_serial.c for #include <../../../nrnconf.h>
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/src/sundials/shared)

# generate hocusr.h
add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/src/nrnoc/hocusr.h
  COMMAND
    ${CMAKE_C_COMPILER} -E -I${NRN_NRNOC_SRC_DIR} -I${NRN_OC_SRC_DIR} ${NRN_NRNOC_SRC_DIR}/neuron.h
    > neuron.tmp1
  COMMAND sed "/^#/d" neuron.tmp1 > neuron.tmp2
  COMMAND python ${NRN_OC_SRC_DIR}/mk_hocusr_h.py < neuron.tmp2 > temp2hoc
  COMMAND sed "s/\\\"nrnhoc_topology\\\"/\\\"topology\\\"/" temp2hoc >
          ${PROJECT_BINARY_DIR}/src/nrnoc/hocusr.h
  DEPENDS ${NRN_NRNOC_SRC_DIR}/neuron.h ${NRN_OC_SRC_DIR}/mk_hocusr_h.py)

# header for dynamic mpi
if(NRN_ENABLE_MPI_DYNAMIC)
  add_custom_command(
    OUTPUT ${NRNMPI_DYNAMIC_INCLUDE_FILE}
    COMMAND sh mkdynam.sh
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/nrnmpi
    DEPENDS ${PROJECT_SOURCE_DIR}/src/nrnmpi/mkdynam.sh
            ${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpidec.h)
endif()

# =============================================================================
# Various macro definitions for compiling different files
# =============================================================================
set_property(
  SOURCE ${NRN_OC_SRC_FILES}
  APPEND
  PROPERTY COMPILE_DEFINITIONS OOP=1 HOC=1 INTERVIEWS=1)

set_property(
  SOURCE ${NRN_NRNOC_SRC_DIR}/code.c ${NRN_NRNOC_SRC_DIR}/hoc_init.c ${NRN_NRNOC_SRC_DIR}/hoc_oop.c
  APPEND
  PROPERTY COMPILE_DEFINITIONS CABLE=1)

set_property(
  SOURCE ${PROJECT_SOURCE_DIR}/src/nrniv/nrnpy.cpp ${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpi_dynam.c
  APPEND
  PROPERTY COMPILE_DEFINITIONS NRNCMAKE)

set_property(
  SOURCE ${PROJECT_SOURCE_DIR}/src/nrniv/nrnpy.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS USE_LIBNRNPYTHON_MAJORMINOR=${USE_LIBNRNPYTHON_MAJORMINOR})

set_property(
  SOURCE ${NRN_IVOC_SRC_FILES}
  APPEND
  PROPERTY COMPILE_DEFINITIONS USEMATRIX=1 USEGNU=1 USEBBS=1)

set_property(
  SOURCE ${NRN_NRNIV_SRC_FILES}
  APPEND
  PROPERTY COMPILE_DEFINITIONS USEMATRIX=1 USECVODE=1 CABLE=1 USEBBS=1 OOP=1)

set_property(
  SOURCE ${NRN_SCOPMATH_SRC_FILES}
  APPEND
  PROPERTY COMPILE_DEFINITIONS HOC=1)

set_property(
  SOURCE ${NRN_SCOPMATH_SRC_FILES}
  APPEND
  PROPERTY COMPILE_FLAGS "-I${PROJECT_SOURCE_DIR}/src/scopmath")

set_source_files_properties(${NRN_NRNOC_SRC_DIR}/code.c PROPERTIES OBJECT_DEPENDS
                            ${NRN_OC_SRC_DIR}/parse.h)

set_source_files_properties(${NRN_NRNOC_SRC_DIR}/nrnversion.c PROPERTIES OBJECT_DEPENDS
                            ${PROJECT_SOURCE_DIR}/src/nrnoc/nrnversion.h)

set_source_files_properties(${NRN_OC_SRC_DIR}/hocusr.c PROPERTIES OBJECT_DEPENDS
                            ${PROJECT_BINARY_DIR}/src/nrnoc/hocusr.h)

if(NRN_ENABLE_MPI_DYNAMIC)
  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpi_dynam.c PROPERTIES
                              OBJECT_DEPENDS ${NRNMPI_DYNAMIC_INCLUDE_FILE})
  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpi.c PROPERTIES OBJECT_DEPENDS
                              ${NRNMPI_DYNAMIC_INCLUDE_FILE})
  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/nrnmpi/bbsmpipack.c PROPERTIES
                              OBJECT_DEPENDS ${NRNMPI_DYNAMIC_INCLUDE_FILE})
endif()

# =============================================================================
# All source directories to include
# =============================================================================
include_directories(${NRN_INCLUDE_DIRS})
if(NRN_ENABLE_PYTHON)
  include_directories(${PYTHON_INCLUDE_DIRS})
endif()

# =============================================================================
# All source directories to include
# =============================================================================
add_library(nrniv_lib SHARED ${NRN_NRNIV_LIB_SRC_FILES})
if(NOT NRN_MACOS_BUILD AND ${READLINE_FOUND})
  target_link_libraries(nrniv_lib ${TERMCAP_LIBRARIES} ${Readline_LIBRARY})
else()
  target_link_libraries(nrniv_lib ${TERMCAP_LIBRARIES} readline)
endif()
set_property(TARGET nrniv_lib PROPERTY OUTPUT_NAME nrniv)

# =============================================================================
# Link with all libraries
# =============================================================================
if(NRN_ENABLE_PYTHON AND NOT NRN_ENABLE_PYTHON_DYNAMIC)
  target_link_libraries(nrniv_lib ${PYTHON_LIBRARIES})
endif()

if(NRN_ENABLE_MPI)
  if(NRN_ENABLE_MPI_DYNAMIC)
    add_library(nrnmpi_lib SHARED ${NRN_NRNMPI_SRC_FILES})
    if(LINK_AGAINST_MPI)
      target_link_libraries(nrnmpi_lib ${MPI_C_LIBRARIES})
    endif()
    if(MINGW)
      add_dependencies(nrnmpi_lib nrniv_lib)
      target_link_libraries(nrnmpi_lib nrniv_lib)
    endif()
    set_property(TARGET nrnmpi_lib PROPERTY OUTPUT_NAME nrnmpi)
    install(TARGETS nrnmpi_lib DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})
  else()
    target_link_libraries(nrniv_lib ${MPI_C_LIBRARIES})
  endif()
endif()

if(NRN_ENABLE_INTERVIEWS)
  include_directories(${IV_INCLUDE_DIR})
  target_link_libraries(nrniv_lib interviews)
else()
  include_directories(nrniv_lib ${NRN_IVOS_SRC_DIR} ${PROJECT_BINARY_DIR}/src/ivos)
endif()

if(IV_ENABLE_X11_DYNAMIC)
  # ~~~
  # by defining IVX11_DYNAM in ivocmain.cpp, the latter can call ivx11_dyload and
  # if that fails, set hoc_usegui = 0 which avoids all InterViews calls.
  # ~~~
  set_property(
    SOURCE ${PROJECT_SOURCE_DIR}/src/ivoc/ivocmain.cpp ${PROJECT_SOURCE_DIR}/src/pwman/xdep.cpp
           ${PROJECT_SOURCE_DIR}/src/ivoc/xdep.cpp ${PROJECT_SOURCE_DIR}/src/oc/x.c
    APPEND
    PROPERTY COMPILE_DEFINITIONS IVX11_DYNAM)
  if((NOT IV_ENABLE_SHARED) AND IV_LIB_DIR)
    # ~~~
    # IV_LIB_DIR is not set when IV is a submodule and not yet installed but
    # libivx11dynam is already in its proper place at POST_BUILD. When
    # IV_ENABLE_SHARED=ON, libivx11dynam is found in IV_LIB_DIR (the location
    # of libinterviews). When OFF, libivx11dynam needs to be copied to the
    # location of libnrniv. The goal is that if libnrniv is loaded, the system
    # can find libivx11dynam independent of CMAKE_INSTALL_PREFIX.
    # ~~~
    set(_suffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(LIBIVX11DYNAM_NAME libivx11dynam${_suffix})
    add_custom_command(
      TARGET nrniv_lib POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${IV_LIB_DIR}/${LIBIVX11DYNAM_NAME}
              ${PROJECT_BINARY_DIR}/lib/${LIBIVX11DYNAM_NAME})
  endif()
else()
  target_link_libraries(nrniv_lib ${X11_LIBRARIES})
endif()

# =============================================================================
# Final executable
# =============================================================================
add_executable(nrniv ${NRN_BIN_SRC_FILES})
target_link_libraries(nrniv nrniv_lib ${INTERNAL_READLINE} ${CMAKE_THREAD_LIBS_INIT})
if(NOT MINGW)
  target_link_libraries(nrniv ${CMAKE_DL_LIBS})
endif()

# TODO: unset in top level CMake is not working
if(NOT NRN_MACOS_BUILD AND ${READLINE_FOUND})
  target_link_libraries(nrniv ${Readline_LIBRARY})
endif()


# =============================================================================
# Install binary and library targets
# =============================================================================
# ~~~
# classically, the autotools windows version installed dlls in <inst>/bin
# For now, we keep this distinction as it reduces the PATH and is
# expected when ctypes looks for dlls
# ~~~
install(TARGETS nrniv nocmodl modlunit DESTINATION bin)
install(TARGETS nrniv_lib DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})
if (LIBIVX11DYNAM_NAME)
  install(FILES ${PROJECT_BINARY_DIR}/lib/${LIBIVX11DYNAM_NAME} DESTINATION lib)
endif()
