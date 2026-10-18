# =============================================================================
# MorphIO
# =============================================================================
#
# NEURON requires MorphIO built with MORPHIO_USE_DOUBLE so that morphology coordinates keep 64-bit
# precision (like Import3d, which stores points in HOC Vectors of double). MorphIO does not export
# this definition to consumers, and morphio::floatType changes the layout of every point/diameter
# range returned by the library, so headers and library must agree. Consumers get the definition
# through nrniv_lib (see src/nrniv/CMakeLists.txt). A pre-installed MorphIO is only used if a run
# probe proves it was built with doubles; otherwise MorphIO is built from source.

set(MorphIO_COMPILE_DEFINITIONS MORPHIO_USE_DOUBLE)

# We search for MorphIO headers and library, otherwise we build it
find_library(MorphIO_LIBRARY NAMES morphio)
find_path(
  MorphIO_INCLUDE_DIR
  NAMES morphio/section.h
  HINTS ${MorphIO_LIBRARY})

set(MorphIO_SYSTEM_USABLE OFF)
if(MorphIO_LIBRARY AND MorphIO_INCLUDE_DIR)
  get_filename_component(MorphIO_LIB_DIR ${MorphIO_LIBRARY} DIRECTORY)
  # Probe: load a one-point SWC through headers compiled with MORPHIO_USE_DOUBLE. With a float build
  # of the library the returned spans are reinterpreted and the exact double check fails.
  set(_morphio_probe_dir "${CMAKE_BINARY_DIR}/CMakeFiles/morphio_double_probe")
  file(WRITE "${_morphio_probe_dir}/probe.swc" "1 1 0.1 0.2 0.3 0.7 -1\n2 3 0.1 1.2 0.3 0.4 1\n")
  file(
    WRITE "${_morphio_probe_dir}/probe.cpp"
    "#include <morphio/morphology.h>\n#include <morphio/soma.h>\n"
    "int main(int, char** argv) {\n"
    "  morphio::Morphology m(argv[1]);\n"
    "  const auto& p = m.soma().points();\n"
    "  if (sizeof(morphio::floatType) != sizeof(double)) return 2;\n"
    "  return (p.size() == 1 && p[0][0] == 0.1 && p[0][1] == 0.2 && p[0][2] == 0.3) ? 0 : 1;\n"
    "}\n")
  try_run(
    _morphio_probe_run _morphio_probe_compile "${_morphio_probe_dir}/build"
    "${_morphio_probe_dir}/probe.cpp"
    CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${MorphIO_INCLUDE_DIR}"
    COMPILE_DEFINITIONS -DMORPHIO_USE_DOUBLE LINK_LIBRARIES ${MorphIO_LIBRARY} CXX_STANDARD 17
    COMPILE_OUTPUT_VARIABLE _morphio_probe_compile_out
    RUN_OUTPUT_VARIABLE _morphio_probe_run_out
    ARGS "${_morphio_probe_dir}/probe.swc")
  if(_morphio_probe_compile AND "${_morphio_probe_run}" STREQUAL "0")
    set(MorphIO_SYSTEM_USABLE ON)
  else()
    message(
      STATUS
        "MorphIO found at ${MorphIO_LIBRARY} but it is not built with MORPHIO_USE_DOUBLE "
        "(compile=${_morphio_probe_compile}, run=${_morphio_probe_run}); building it from source")
    unset(MorphIO_LIBRARY CACHE)
    unset(MorphIO_INCLUDE_DIR CACHE)
  endif()
endif()

if(MorphIO_SYSTEM_USABLE)
  message(STATUS "MorphIO Lib and Includes found: ${MorphIO_LIBRARY} ${MorphIO_INCLUDE_DIR}")
  set(MorphIO_FOUND ON)
  add_library(MorphIO INTERFACE IMPORTED)
else()
  include(ExternalProject)
  set(MorphIO_PREFIX "${CMAKE_BINARY_DIR}/external/MorphIO")
  set(MorphIO_INCLUDE_DIR "${MorphIO_PREFIX}/include")
  set(MorphIO_LIB_DIR "${MorphIO_PREFIX}/lib")
  file(MAKE_DIRECTORY "${MorphIO_INCLUDE_DIR}")

  set(MorphIO_C_FLAGS ${CMAKE_C_FLAGS})
  list(REMOVE_ITEM MorphIO_C_FLAGS "-tp=haswell")
  set(MorphIO_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  list(REMOVE_ITEM MorphIO_CXX_FLAGS "-tp=haswell")
  if(NOT PYTHON_EXECUTABLE)
    set(PYTHON_EXECUTABLE ${NRN_DEFAULT_PYTHON_EXECUTABLE})
  endif()
  ExternalProject_Add(
    morphio-external
    BINARY_DIR "${CMAKE_BINARY_DIR}/external/MorphIO/build"
    PREFIX "${MorphIO_PREFIX}"
    GIT_REPOSITORY https://github.com/openbraininstitute/MorphIO.git
    GIT_TAG v3.3.5
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    UPDATE_COMMAND
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
               -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
               -DCMAKE_POLICY_VERSION_MINIMUM=3.5
               -DBUILD_BINDINGS=OFF
               -DMORPHIO_TESTS=OFF
               -DCMAKE_C_FLAGS=${MorphIO_C_FLAGS}
               -DCMAKE_CXX_FLAGS=${MorphIO_CXX_FLAGS}
               -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
               -DCMAKE_INSTALL_LIBDIR=lib
               -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
               -DMORPHIO_USE_DOUBLE=ON
    BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libmorphio${CMAKE_SHARED_LIBRARY_SUFFIX})

  install(FILES ${MorphIO_LIB_DIR}/libmorphio${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION lib)

  add_custom_command(
    TARGET morphio-external
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${MorphIO_LIB_DIR}/libmorphio${CMAKE_SHARED_LIBRARY_SUFFIX}
            ${CMAKE_BINARY_DIR}/lib)
  add_library(MorphIO INTERFACE IMPORTED)
  add_dependencies(MorphIO morphio-external)
endif()
