# =============================================================================
# Copy necessary files to build directory
# =============================================================================

cpp_cc_build_time_copy(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/newton/newton.hpp" OUTPUT
                       "${CMAKE_BINARY_DIR}/include/newton/newton.hpp")

cpp_cc_build_time_copy(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/crout/crout.hpp" OUTPUT
                       "${CMAKE_BINARY_DIR}/include/crout/crout.hpp")

# Eigen
file(COPY ${NMODL_PROJECT_SOURCE_DIR}/ext/eigen/Eigen DESTINATION ${CMAKE_BINARY_DIR}/include/)

# =============================================================================
# Install solver headers and eigen from include
# =============================================================================

install(DIRECTORY ${CMAKE_BINARY_DIR}/include/ DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}include)
