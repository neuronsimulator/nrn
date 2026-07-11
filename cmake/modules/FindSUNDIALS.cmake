# cmake/modules/FindSUNDIALS.cmake

set(SUNDIALS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/external/sundials")

# Use the same pattern as other submodules in NEURON
if(NOT EXISTS "${SUNDIALS_SOURCE_DIR}/CMakeLists.txt")
  message(STATUS "SUNDIALS submodule not found.")
  include(${PROJECT_SOURCE_DIR}/cmake/ExternalProjectHelper.cmake)
  nrn_initialize_submodule(external/sundials)
endif()

if(NOT EXISTS "${SUNDIALS_SOURCE_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "Failed to initialize SUNDIALS submodule")
endif()

set(SUNDIALS_PREFIX "${CMAKE_BINARY_DIR}/external/sundials")
set(SUNDIALS_INCLUDE_DIR "${SUNDIALS_PREFIX}/include")
set(SUNDIALS_LIB_DIR "${SUNDIALS_PREFIX}/lib")

include(ExternalProject)
ExternalProject_Add(
  sundials
  SOURCE_DIR "${SUNDIALS_SOURCE_DIR}"
  PREFIX "${SUNDIALS_PREFIX}"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_POSITION_INDEPENDENT_CODE=ON
             -DBUILD_SHARED_LIBS=OFF
             -DEXAMPLES_ENABLE_C=OFF
             -DEXAMPLES_INSTALL=OFF
             -DBUILD_ARKODE=OFF
             -DBUILD_CVODES=OFF
             -DBUILD_IDAS=OFF
             -DBUILD_KINSOL=OFF
             -DMPI_ENABLE=OFF
             -DPTHREAD_ENABLE=ON
             -DSUNDIALS_PRECISION=double
             -DUSE_GENERIC_MATH=ON
  BUILD_BYPRODUCTS
    <INSTALL_DIR>/lib/libsundials_cvode.a <INSTALL_DIR>/lib/libsundials_ida.a
    <INSTALL_DIR>/lib/libsundials_nvecserial.a <INSTALL_DIR>/lib/libsundials_nvecpthreads.a
  INSTALL_DIR "${SUNDIALS_PREFIX}")

add_library(SUNDIALS INTERFACE IMPORTED)
add_dependencies(SUNDIALS sundials)

set(SUNDIALS_INCLUDE_DIR
    "${SUNDIALS_INCLUDE_DIR}"
    PARENT_SCOPE)
set(SUNDIALS_LIB_DIR
    "${SUNDIALS_LIB_DIR}"
    PARENT_SCOPE)
