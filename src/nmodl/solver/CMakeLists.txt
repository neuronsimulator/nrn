# =============================================================================
# Solver sources
# =============================================================================
set(SOLVER_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/newton/newton.hpp)

# =============================================================================
# Copy necessary headers to build directory
# =============================================================================
file(GLOB NMODL_SOLVER_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/newton/*.h*")
file(COPY ${NMODL_SOLVER_HEADER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/include/newton/)
file(COPY ${NMODL_PROJECT_SOURCE_DIR}/ext/eigen/Eigen DESTINATION ${CMAKE_BINARY_DIR}/include/)

# =============================================================================
# Install solver headers and eigen from include
# =============================================================================
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/ DESTINATION include)
