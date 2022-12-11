# =============================================================================
# Sundials
# =============================================================================

include(ExternalProject)
set(SUNDIALS_PREFIX "${THIRD_PARTY_DIRECTORY}/sundials")
set(SUNDIALS_INCLUDE_DIR "${SUNDIALS_PREFIX}/include")
set(SUNDIALS_LIB_DIR "${SUNDIALS_PREFIX}/lib")
make_directory(${SUNDIALS_INCLUDE_DIR})

include(ExternalProject)
ExternalProject_Add(
  sundials-external
  PREFIX "${SUNDIALS_PREFIX}"
  GIT_REPOSITORY https://github.com/LLNL/sundials.git
  # GIT_TAG e2f29c34f324829302037a1492db480be8828084   6.2.0 -> CVodeMem no longer "visible"
  # GIT_TAG c09e732080a214694b209032ec627c93fed45340  4
  GIT_TAG 811234254d37652954daff0ccdb7af9813736846 # 3.2.1
  # GIT_TAG 7ed895fd102226dbc52225e6b2c6e26ef1cafa0e  #2.7.0
  GIT_SHALLOW ON
  GIT_PROGRESS ON
  # UPDATE_COMMAND ""
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
             -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DEXAMPLES_INSTALL=OFF
             -DBUILD_ARKODE=OFF
             -DBUILD_CVODES=OFF
             -DBUILD_IDAS=OFF
             -DBUILD_KINSOL=OFF
             -DEXAMPLES_ENABLE_C=OFF
             -DMPI_ENABLE=ON
             -DPTHREAD_ENABLE=ON
             -DSUNDIALS_PRECISION=double
             -DUSE_GENERIC_MATH=ON
             -DCMAKE_BUILD_TYPE=RelWitDebInfo
             -DCMAKE_POSITION_INDEPENDENT_CODE=ON
             -DBUILD_SHARED_LIBS=OFF
  BUILD_BYPRODUCTS
    <INSTALL_DIR>/lib/libsundials_ida.a <INSTALL_DIR>/lib/libsundials_cvode.a
    <INSTALL_DIR>/lib/libsundials_nvecserial.a <INSTALL_DIR>/lib/libsundials_nvecpthreads.a
    <INSTALL_DIR>/lib/libsundials_nvecparallel.a)

add_library(SUNDIALS INTERFACE IMPORTED)
add_dependencies(SUNDIALS sundials-external)

set(SUNDIALS_HOME ${SUNDIALS_PREFIX})

include_directories(${SUNDIALS_INCLUDE_DIR})
link_directories(${SUNDIALS_LIB_DIR})

set_target_properties(
  SUNDIALS
  PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${SUNDIALS_INCLUDE_DIR}
             INTERFACE_INCLUDE_DIRECTORIES ${SUNDIALS_INCLUDE_DIR}
             INTERFACE_LINK_LIBRARIES ${SUNDIALS_LIB_DIR})
