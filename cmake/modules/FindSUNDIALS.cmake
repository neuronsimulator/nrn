# =============================================================================
# Sundials
# =============================================================================

include(ExternalProject)
set(SUNDIALS_PREFIX "${THIRD_PARTY_DIRECTORY}/sundials")
set(SUNDIALS_INCLUDE_DIR "${SUNDIALS_PREFIX}/include")
set(SUNDIALS_LIB_DIR "${SUNDIALS_PREFIX}/lib")
make_directory(${SUNDIALS_INCLUDE_DIR})

set(SUNDIALS_C_FLAGS ${CMAKE_C_FLAGS})
list(REMOVE_ITEM SUNDIALS_C_FLAGS "-tp=haswell")
set(SUNDIALS_CXX_FLAGS ${CMAKE_CXX_FLAGS})
list(REMOVE_ITEM SUNDIALS_CXX_FLAGS "-tp=haswell")
include(ExternalProject)
ExternalProject_Add(
  sundials-external
  PREFIX "${SUNDIALS_PREFIX}"
  # cmake-format: off
  # GIT_REPOSITORY https://github.com/LLNL/sundials.git
  # Forked to following, to allow RPowerR to use hoc_pow
  GIT_REPOSITORY https://github.com/neuronsimulator/sundials.git
  # GIT_TAG e2f29c34f324829302037a1492db480be8828084   6.2.0 -> CVodeMem no longer "visible" GIT_TAG
  # c09e732080a214694b209032ec627c93fed45340  4
  #GIT_TAG 811234254d37652954daff0ccdb7af9813736846 # 3.2.1
  GIT_TAG 71b3daec18ce66a7c011be9501a31567aa5c09b6 # 3.2.1 hines/hoc_pow
  # GIT_TAG 7ed895fd102226dbc52225e6b2c6e26ef1cafa0e  #2.7.0
  # cmake-format: on
  GIT_SHALLOW ON
  GIT_PROGRESS ON
  # UPDATE_COMMAND ""
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SUNDIALS_C_FLAGS}
             -DCMAKE_CXX_FLAGS=${SUNDIALS_CXX_FLAGS}
             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DEXAMPLES_INSTALL=OFF
             -DBUILD_ARKODE=OFF
             -DBUILD_CVODES=OFF
             -DBUILD_IDAS=OFF
             -DBUILD_KINSOL=OFF
             -DEXAMPLES_ENABLE_C=OFF
             -DMPI_ENABLE=OFF
             -DPTHREAD_ENABLE=ON
             -DSUNDIALS_PRECISION=double
             -DUSE_GENERIC_MATH=ON
             -DCMAKE_POSITION_INDEPENDENT_CODE=ON
             -DBUILD_SHARED_LIBS=OFF
             -DMPI_C_COMPILER=${MPI_C_COMPILER}
             -DCMAKE_INSTALL_LIBDIR=${SUNDIALS_LIB_DIR}
  BUILD_BYPRODUCTS
    <INSTALL_DIR>/lib/libsundials_ida.a <INSTALL_DIR>/lib/libsundials_cvode.a
    <INSTALL_DIR>/lib/libsundials_nvecserial.a <INSTALL_DIR>/lib/libsundials_nvecpthreads.a
    <INSTALL_DIR>/lib/libsundials_nvecparallel.a)

add_library(SUNDIALS INTERFACE IMPORTED)
add_dependencies(SUNDIALS sundials-external)
