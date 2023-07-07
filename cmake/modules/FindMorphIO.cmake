# =============================================================================
# MorphIO
# =============================================================================

include(ExternalProject)
set(MorphIO_PREFIX "${THIRD_PARTY_DIRECTORY}/MorphIO")
set(MorphIO_INCLUDE_DIR "${MorphIO_PREFIX}/include")
set(MorphIO_LIB_DIR "${MorphIO_PREFIX}/lib")
make_directory(${MorphIO_INCLUDE_DIR})

set(MorphIO_C_FLAGS ${CMAKE_C_FLAGS})
list(REMOVE_ITEM MorphIO_C_FLAGS "-tp=haswell")
set(MorphIO_CXX_FLAGS ${CMAKE_CXX_FLAGS})
list(REMOVE_ITEM MorphIO_CXX_FLAGS "-tp=haswell")
include(ExternalProject)
ExternalProject_Add(
  morphio-external
  BINARY_DIR "${CMAKE_BINARY_DIR}/external/MorphIO"
  BUILD_ALWAYS ON
  PREFIX "${MorphIO_PREFIX}"
  GIT_REPOSITORY https://github.com/BlueBrain/MorphIO.git
  GIT_TAG v3.3.5
  GIT_SHALLOW ON
  GIT_PROGRESS ON
  UPDATE_COMMAND
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_C_FLAGS=${MorphIO_C_FLAGS}
             -DCMAKE_CXX_FLAGS=${MorphIO_CXX_FLAGS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_LIBDIR=${MorphIO_LIB_DIR} -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
  BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libmorphio${CMAKE_SHARED_LIBRARY_SUFFIX})

install(FILES ${MorphIO_PREFIX}/lib/libmorphio${CMAKE_SHARED_LIBRARY_SUFFIX}
        DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_custom_command(
  TARGET morphio-external
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${MorphIO_PREFIX}/lib/libmorphio${CMAKE_SHARED_LIBRARY_SUFFIX}
          ${CMAKE_BINARY_DIR}/lib)
add_library(MorphIO INTERFACE IMPORTED)
add_dependencies(MorphIO morphio-external)
