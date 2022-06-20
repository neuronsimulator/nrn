# =============================================================================
# Build nrniv binary and corresponding library
# =============================================================================

# Add directory-level default compiler flags -- these should be added to all NEURON targets, but not
# targets from included projects like CoreNEURON and NMODL
add_compile_options(${NRN_COMPILE_FLAGS})
add_compile_definitions(${NRN_COMPILE_DEFS})
add_link_options(${NRN_LINK_FLAGS})

if(NRN_AVOID_ABSOLUTE_PATHS)
  add_compile_definitions(NRN_AVOID_ABSOLUTE_PATHS)
endif()

# =============================================================================
# Build modlunit : Mod file units checker
# =============================================================================
set(NRN_MODLUNIT_GEN "${CMAKE_CURRENT_BINARY_DIR}/modlunit_generated")
file(MAKE_DIRECTORY "${NRN_MODLUNIT_GEN}")
file(RELATIVE_PATH NRN_MODLUNIT_SRC_REL "${NRN_MODLUNIT_GEN}" "${NRN_MODLUNIT_SRC_DIR}")
# Run flex and bison with relative paths, so absolute paths are not present in the generated source
# file because of include, __FILE__ and so on. This improves ccache performance.
add_custom_command(
  OUTPUT "${NRN_MODLUNIT_GEN}/lex.cpp"
  WORKING_DIRECTORY "${NRN_MODLUNIT_GEN}"
  COMMAND "${FLEX_EXECUTABLE}" ARGS -o lex.cpp "${NRN_MODLUNIT_SRC_REL}/lex.lpp"
  DEPENDS "${NRN_MODLUNIT_SRC_DIR}/lex.lpp"
  COMMENT "[FLEX][modlunitlexer] Building scanner with flex ${FLEX_VERSION}")
add_custom_command(
  OUTPUT "${NRN_MODLUNIT_GEN}/parse1.hpp" "${NRN_MODLUNIT_GEN}/parse1.cpp"
  WORKING_DIRECTORY "${NRN_MODLUNIT_GEN}"
  COMMAND "${BISON_EXECUTABLE}" ARGS --defines=parse1.hpp -o parse1.cpp
          "${NRN_MODLUNIT_SRC_REL}/parse1.ypp"
  DEPENDS "${NRN_MODLUNIT_SRC_DIR}/parse1.ypp"
  COMMENT "[BISON][modlunitparser] Building parser with bison ${BISON_VERSION}")

add_custom_target(generated_source_files true)
add_custom_target(modlunit_generated_files DEPENDS "${NRN_MODLUNIT_GEN}/lex.cpp"
                                                   "${NRN_MODLUNIT_GEN}/parse1.cpp")
add_dependencies(generated_source_files modlunit_generated_files)
add_executable(modlunit ${NRN_MODLUNIT_SRC_FILES} "${NRN_MODLUNIT_GEN}/lex.cpp"
                        "${NRN_MODLUNIT_GEN}/parse1.cpp")
target_compile_definitions(modlunit PRIVATE NRNUNIT=1)
cpp_cc_configure_sanitizers(TARGET modlunit)
# Generated .cpp needs to find source-directory .hpp and vice versa.
target_include_directories(modlunit PRIVATE "${NRN_MODLUNIT_SRC_DIR}" "${NRN_MODLUNIT_GEN}")

if(NRN_NMODL_CXX_FLAGS)
  target_compile_options(modlunit PRIVATE ${NRN_NMODL_CXX_FLAGS})
endif()

# =============================================================================
# Build nocmodl : source-to-source compiler for NMODL
# =============================================================================
set(NRN_NMODL_GEN "${CMAKE_CURRENT_BINARY_DIR}/nocmodl_generated")
file(MAKE_DIRECTORY "${NRN_NMODL_GEN}")
file(RELATIVE_PATH NRN_NMODL_SRC_REL "${NRN_NMODL_GEN}" "${NRN_NMODL_SRC_DIR}")
add_custom_command(
  OUTPUT "${NRN_NMODL_GEN}/lex.cpp"
  WORKING_DIRECTORY "${NRN_NMODL_GEN}"
  COMMAND "${FLEX_EXECUTABLE}" ARGS -o lex.cpp "${NRN_NMODL_SRC_REL}/lex.lpp"
  DEPENDS "${NRN_NMODL_SRC_DIR}/lex.lpp"
  COMMENT "[FLEX][nocmodllexer] Building scanner with flex ${FLEX_VERSION}")
add_custom_command(
  OUTPUT "${NRN_NMODL_GEN}/parse1.hpp" "${NRN_NMODL_GEN}/parse1.cpp"
  WORKING_DIRECTORY "${NRN_NMODL_GEN}"
  COMMAND "${BISON_EXECUTABLE}" ARGS --defines=parse1.hpp -o parse1.cpp
          "${NRN_NMODL_SRC_REL}/parse1.ypp"
  DEPENDS "${NRN_NMODL_SRC_DIR}/parse1.ypp"
  COMMENT "[BISON][nocmodlparser] Building parser with bison ${BISON_VERSION}")
add_custom_command(
  OUTPUT "${NRN_NMODL_GEN}/diffeq.hpp" "${NRN_NMODL_GEN}/diffeq.cpp"
  WORKING_DIRECTORY "${NRN_NMODL_GEN}"
  COMMAND "${BISON_EXECUTABLE}" ARGS --defines=diffeq.hpp -o diffeq.cpp
          "${NRN_NMODL_SRC_REL}/diffeq.ypp"
  DEPENDS "${NRN_NMODL_SRC_DIR}/diffeq.ypp"
  COMMENT "[BISON][nocmodlparser] Building parser with bison ${BISON_VERSION}")

add_custom_target(
  nocmodl_generated_files
  DEPENDS "${NRN_NMODL_GEN}/lex.cpp" "${NRN_NMODL_GEN}/parse1.hpp" "${NRN_NMODL_GEN}/parse1.cpp"
          "${NRN_NMODL_GEN}/diffeq.hpp" "${NRN_NMODL_GEN}/diffeq.cpp")
add_dependencies(generated_source_files nocmodl_generated_files)
add_executable(nocmodl ${NRN_NMODL_SRC_FILES} "${NRN_NMODL_GEN}/lex.cpp"
                       "${NRN_NMODL_GEN}/parse1.cpp" "${NRN_NMODL_GEN}/diffeq.cpp")
cpp_cc_configure_sanitizers(TARGET nocmodl)
target_compile_definitions(nocmodl PRIVATE COMPILE_DEFINITIONS NMODL=1 NOCMODL=1 CVODE=1
                                           VECTORIZE=1 NRN_DYNAMIC_UNITS=1)
# Otherwise the generated code in the binary directory does not find headers in the modlunit source
# directory and the source files in the source directory do not find generated headers in the binary
# directory.
target_include_directories(nocmodl PRIVATE "${NRN_NMODL_GEN}" "${NRN_NMODL_SRC_DIR}")
if(NRN_NMODL_CXX_FLAGS)
  target_compile_options(nocmodl PRIVATE ${NRN_NMODL_CXX_FLAGS})
endif()

# =============================================================================
# Translate all MOD files to C and mark them generated
# =============================================================================
foreach(modfile ${NRN_MODFILE_BASE_NAMES})
  nocmodl_mod_to_cpp(${modfile})
  set_property(
    SOURCE ${modfile}.cpp
    APPEND
    PROPERTY COMPILE_OPTIONS ${NRN_EXTRA_MECH_CXX_FLAGS})
  list(APPEND NRN_MODFILE_CPP ${modfile}.cpp)
endforeach()

set_source_files_properties(${NRN_MODFILE_CPP} PROPERTIES GENERATED TRUE)

# =============================================================================
# Bison parser for HOC interpreter
# =============================================================================
set(NRN_OC_GEN "${CMAKE_CURRENT_BINARY_DIR}/oc_generated")
file(MAKE_DIRECTORY "${NRN_OC_GEN}")
file(RELATIVE_PATH NRN_OC_SRC_REL "${NRN_OC_GEN}" "${NRN_OC_SRC_DIR}")
add_custom_command(
  OUTPUT "${NRN_OC_GEN}/parse.hpp" "${NRN_OC_GEN}/parse.cpp"
  WORKING_DIRECTORY "${NRN_OC_GEN}"
  COMMAND "${BISON_EXECUTABLE}" ARGS --defines=parse.hpp -o parse.cpp "${NRN_OC_SRC_REL}/parse.ypp"
  DEPENDS "${NRN_OC_SRC_DIR}/parse.ypp"
  COMMENT "[BISON][ocparser] Building parser with bison ${BISON_VERSION}")
add_custom_target(oc_generated_files DEPENDS "${NRN_OC_GEN}/parse.hpp" "${NRN_OC_GEN}/parse.cpp")
add_dependencies(generated_source_files oc_generated_files)
list(APPEND NRN_OC_SRC_FILES "${NRN_OC_GEN}/parse.cpp")
set(NRN_OC_GENERATED_SOURCES
    "${NRN_OC_GEN}"
    PARENT_SCOPE)

# =============================================================================
# Source code lists
# =============================================================================
set(NRN_NRNIV_LIB_SRC_FILES
    ${NRN_IVOC_SRC_FILES}
    ${NRN_NRNCVODE_SRC_FILES}
    ${NRN_NRNIV_SRC_FILES}
    ${NRN_NRNOC_SRC_FILES}
    ${NRN_OC_SRC_FILES}
    ${NRN_MESCH_SRC_FILES}
    ${NRN_MODFILE_CPP}
    ${NRN_NRNGNU_SRC_FILES}
    ${NRN_SCOPMATH_SRC_FILES}
    ${NRN_SPARSE_SRC_FILES}
    ${NRN_SPARSE13_SRC_FILES}
    ${NRN_SUNDIALS_SRC_FILES})

if(NRN_ENABLE_MPI)
  list(APPEND NRN_NRNIV_LIB_SRC_FILES ${NRN_PARALLEL_SRC_FILES})
endif()

if(NRN_ENABLE_MPI_DYNAMIC)
  list(APPEND NRN_NRNIV_LIB_SRC_FILES ${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpi_dynam.cpp)
else()
  list(APPEND NRN_NRNIV_LIB_SRC_FILES ${NRN_NRNMPI_SRC_FILES})
endif()

if(MINGW)
  list(APPEND NRN_NRNIV_LIB_SRC_FILES ${PROJECT_SOURCE_DIR}/src/mswin/dlfcn.c)
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
    ${NRN_NRNCVODE_SRC_DIR}
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
    ${PROJECT_SOURCE_DIR}/external/Random123/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/gnu
    ${PROJECT_SOURCE_DIR}/src/mesch
    ${PROJECT_SOURCE_DIR}/src/nrncvode
    ${PROJECT_SOURCE_DIR}/src/nrnmpi
    ${PROJECT_SOURCE_DIR}/src/nrnpython
    ${PROJECT_SOURCE_DIR}/src/parallel
    ${PROJECT_SOURCE_DIR}/src/scopmath
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
  nrnversion_h
  COMMAND ${CMAKE_COMMAND} -E env PROJECT_VERSION=${PROJECT_VERSION} bash
          ${PROJECT_SOURCE_DIR}/git2nrnversion_h.sh ${PROJECT_SOURCE_DIR} > nrnversion.h.tmp
  COMMAND ${CMAKE_COMMAND} -E copy_if_different nrnversion.h.tmp ${NRN_NRNOC_SRC_DIR}/nrnversion.h
  DEPENDS ${PROJECT_SOURCE_DIR}/git2nrnversion_h.sh)

add_custom_command(OUTPUT ${NRN_NRNOC_SRC_DIR}/nrnversion.h DEPENDS nrnversion_h)
add_dependencies(generated_source_files nrnversion_h)

# avoid error with nvector_serial.cpp for #include <../../../nrnconf.h>
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/src/sundials/shared)

# generate hocusr.h
add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/src/oc/hocusr.h
  COMMAND ${CMAKE_C_COMPILER} -E -I${NRN_NRNOC_SRC_DIR} -I${NRN_OC_SRC_DIR}
          ${NRN_NRNOC_SRC_DIR}/neuron.h > neuron.tmp1
  COMMAND sed "/^#/d" neuron.tmp1 > neuron.tmp2
  COMMAND ${PYTHON_EXECUTABLE} ${NRN_OC_SRC_DIR}/mk_hocusr_h.py < neuron.tmp2 >
          ${PROJECT_BINARY_DIR}/src/oc/hocusr.h
  DEPENDS ${NRN_NRNOC_SRC_DIR}/neuron.h ${NRN_OC_SRC_DIR}/mk_hocusr_h.py)
add_custom_target(generate_hocusr_header DEPENDS "${PROJECT_BINARY_DIR}/src/oc/hocusr.h")
add_dependencies(generated_source_files generate_hocusr_header)

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
  SOURCE ${NRN_OC_SRC_FILES} ${NRN_SPARSE_SRC_FILES}
  APPEND
  PROPERTY COMPILE_DEFINITIONS OOP=1 HOC=1 INTERVIEWS=1)

set_property(
  SOURCE ${NRN_OC_SRC_DIR}/code.cpp ${NRN_OC_SRC_DIR}/hoc_init.cpp ${NRN_OC_SRC_DIR}/hoc_oop.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS CABLE=1)

set_property(
  SOURCE ${PROJECT_SOURCE_DIR}/src/nrniv/nrnpy.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS USE_LIBNRNPYTHON_MAJORMINOR=${USE_LIBNRNPYTHON_MAJORMINOR})

set_property(
  SOURCE ${NRN_IVOC_SRC_FILES}
  APPEND
  PROPERTY COMPILE_DEFINITIONS USEMATRIX=1 USEGNU=1 USEBBS=1)

set_property(
  SOURCE ${NRN_NRNIV_SRC_FILES} ${NRN_NRNCVODE_SRC_FILES}
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

if(DEF_RL_GETC_FUNCTION)
  set_property(
    SOURCE ${NRN_OC_SRC_DIR}/hoc.cpp
    APPEND
    PROPERTY COMPILE_DEFINITIONS ${DEF_RL_GETC_FUNCTION})
endif()

if(NRN_USE_BACKWARD)
  set_property(
    SOURCE ${NRN_OC_SRC_DIR}/hoc.cpp
    APPEND
    PROPERTY COMPILE_DEFINITIONS USE_BACKWARD=1)
  set_property(
    SOURCE ${NRN_NRNIV_SRC_DIR}/backtrace_utils.cpp
    APPEND
    PROPERTY COMPILE_DEFINITIONS USE_BACKWARD=1)
endif()

# NVHPC/21.7 cannot compile znorm.c with -O2 or above. See also:
# https://forums.developer.nvidia.com/t/nvc-21-7-regression-internal-compiler-error-can-only-coerce-indirect-args/184847
if(NRN_HAVE_NVHPC_COMPILER AND ${CMAKE_C_COMPILER_VERSION} VERSION_EQUAL 21.7)
  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/mesch/znorm.c PROPERTIES COMPILE_OPTIONS
                                                                                 -Mnovect)
endif()

if(NRN_ENABLE_MPI_DYNAMIC)
  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpi_dynam.cpp
                              PROPERTIES OBJECT_DEPENDS ${NRNMPI_DYNAMIC_INCLUDE_FILE})
  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/nrnmpi/nrnmpi.cpp
                              PROPERTIES OBJECT_DEPENDS ${NRNMPI_DYNAMIC_INCLUDE_FILE})
  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/nrnmpi/bbsmpipack.cpp
                              PROPERTIES OBJECT_DEPENDS ${NRNMPI_DYNAMIC_INCLUDE_FILE})
endif()

if(NRN_COVERAGE_FILES)
  foreach(f ${NRN_COVERAGE_FILES})
    set_property(
      SOURCE ${PROJECT_SOURCE_DIR}/${f}
      APPEND
      PROPERTY COMPILE_FLAGS ${NRN_COVERAGE_FLAGS})
  endforeach()
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
add_library(nrniv_lib ${NRN_LIBRARY_TYPE} ${NRN_NRNIV_LIB_SRC_FILES})
cpp_cc_configure_sanitizers(TARGET nrniv_lib)
# Source-directory .cpp needs to find generated .hpp.
target_include_directories(nrniv_lib PUBLIC "${NRN_OC_GEN}")
if(${USE_PTHREAD} EQUAL 1)
  target_link_libraries(nrniv_lib Threads::Threads)
endif()
if(NRN_WINDOWS_BUILD)
  target_link_libraries(nrniv_lib ${TERMCAP_LIBRARIES} ${Readline_LIBRARY})
else()
  if(READLINE_FOUND)
    target_link_libraries(nrniv_lib ${Readline_LIBRARY})
  else()
    target_link_libraries(nrniv_lib readline)
  endif()

  if(CURSES_FOUND)
    target_link_libraries(nrniv_lib ${CURSES_LIBRARIES})
  elseif(TERMCAP_FOUND)
    target_link_libraries(nrniv_lib ${TERMCAP_LIBRARIES})
  endif()
endif()

if(NRN_ENABLE_PROFILING)
  target_link_libraries(nrniv_lib ${likwid_LIBRARIES} ${CALIPER_LIB})
endif()

set_property(TARGET nrniv_lib PROPERTY OUTPUT_NAME nrniv)

# =============================================================================
# Link with backward-cpp if enabled
# =============================================================================
if(NRN_USE_BACKWARD)
  add_backward(nrniv_lib)
endif()

# =============================================================================
# Link with all libraries
# =============================================================================
if(NRN_ENABLE_PYTHON AND NOT NRN_ENABLE_PYTHON_DYNAMIC)
  target_link_libraries(nrniv_lib ${PYTHON_LIBRARIES})
endif()

if(NRN_ENABLE_MPI)
  if(NRN_ENABLE_MPI_DYNAMIC)
    list(LENGTH NRN_MPI_LIBNAME_LIST _num_mpi)
    math(EXPR num_mpi "${_num_mpi} - 1")
    foreach(val RANGE ${num_mpi})
      list(GET NRN_MPI_INCLUDE_LIST ${val} include)
      list(GET NRN_MPI_LIBNAME_LIST ${val} libname)

      add_library(${libname}_lib SHARED ${NRN_NRNMPI_SRC_FILES})
      target_include_directories(${libname}_lib BEFORE PUBLIC ${include})
      # Note that we do not link here to libmpi. That is dlopen first.
      if(MINGW) # type msmpi only
        add_dependencies(${libname}_lib nrniv_lib)
        target_link_libraries(${libname}_lib ${MPI_C_LIBRARIES})
        target_link_libraries(${libname}_lib nrniv_lib)
      endif()
      set_property(TARGET ${libname}_lib PROPERTY OUTPUT_NAME ${libname})
      install(TARGETS ${libname}_lib DESTINATION ${NRN_INSTALL_SHARE_LIB_DIR})
    endforeach(val)
  else()
    target_link_libraries(nrniv_lib ${MPI_C_LIBRARIES})
    target_include_directories(nrniv_lib PUBLIC ${MPI_INCLUDE_PATH})
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
           ${PROJECT_SOURCE_DIR}/src/ivoc/xdep.cpp ${PROJECT_SOURCE_DIR}/src/oc/x.cpp
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
      TARGET nrniv_lib
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${IV_LIB_DIR}/${LIBIVX11DYNAM_NAME}
              ${PROJECT_BINARY_DIR}/lib/${LIBIVX11DYNAM_NAME})
  endif()
else()
  target_link_libraries(nrniv_lib ${X11_LIBRARIES})
endif()

if(NRN_COVERAGE_FILES)
  target_link_libraries(nrniv_lib ${NRN_COVERAGE_LIB})
  target_link_libraries(modlunit ${NRN_COVERAGE_LIB})
  target_link_libraries(nocmodl ${NRN_COVERAGE_LIB})
endif()

# =============================================================================
# Final executable
# =============================================================================
add_executable(nrniv ${NRN_BIN_SRC_FILES})
cpp_cc_configure_sanitizers(TARGET nrniv)
target_link_libraries(nrniv nrniv_lib ${INTERNAL_READLINE} ${CMAKE_DL_LIBS})
if(${USE_PTHREAD} EQUAL 1)
  target_link_libraries(nrniv Threads::Threads)
endif()
if(NOT MINGW)
  target_link_libraries(nrniv ${CMAKE_DL_LIBS})
endif()

# TODO: unset in top level CMake is not working
if(NOT NRN_MACOS_BUILD AND READLINE_FOUND)
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
if(LIBIVX11DYNAM_NAME)
  install(FILES ${PROJECT_BINARY_DIR}/lib/${LIBIVX11DYNAM_NAME} DESTINATION lib)
endif()

# =============================================================================
# Install / copy cpp and required headers for binary special flavor
# =============================================================================

# For testneuron CTest
file(COPY ${PROJECT_SOURCE_DIR}/src/ivoc/nrnmain.cpp DESTINATION ${CMAKE_BINARY_DIR}/share/nrn)
file(COPY ${PROJECT_BINARY_DIR}/src/oc/nrnmpiuse.h DESTINATION ${CMAKE_BINARY_DIR}/include)
file(COPY ${PROJECT_BINARY_DIR}/src/nrncvode/nrnneosm.h
     DESTINATION ${CMAKE_BINARY_DIR}/include/nrncvode)
file(COPY ${PROJECT_BINARY_DIR}/nrnconf.h DESTINATION ${CMAKE_BINARY_DIR}/include)

# For the installation
install(FILES ${PROJECT_SOURCE_DIR}/src/ivoc/nrnmain.cpp DESTINATION share/nrn)
install(FILES ${PROJECT_BINARY_DIR}/src/oc/nrnmpiuse.h DESTINATION include)
install(FILES ${PROJECT_BINARY_DIR}/src/nrncvode/nrnneosm.h DESTINATION include/nrncvode)
install(FILES ${PROJECT_BINARY_DIR}/nrnconf.h DESTINATION include)
