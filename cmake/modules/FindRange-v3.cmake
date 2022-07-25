include(ExternalProject)
set(RANGE_V3_PREFIX "${THIRD_PARTY_DIRECTORY}/range-v3")
set(RANGE_V3_INCLUDE_DIR "${RANGE_V3_PREFIX}/include")
make_directory(${RANGE_V3_INCLUDE_DIR})

ExternalProject_Add(
  range-v3-external
  PREFIX "${CMAKE_BINARY_DIR}"
  URL https://github.com/ericniebler/range-v3/archive/refs/tags/0.12.0.tar.gz
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${RANGE_V3_PREFIX}
             -DCMAKE_BUILD_TYPE="Release"
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DRANGES_CXX_STD=${CMAKE_CXX_STANDARD}
             -DRANGE_V3_DOCS=OFF
             -DRANGE_V3_TESTS=OFF
             -DRANGE_V3_EXAMPLES=OFF)
add_library(range-v3 INTERFACE IMPORTED)
set_target_properties(
  range-v3 PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${RANGE_V3_INCLUDE_DIR}
                      INTERFACE_INCLUDE_DIRECTORIES ${RANGE_V3_INCLUDE_DIR})
add_dependencies(range-v3 range-v3-external)
