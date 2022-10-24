# =============================================================================
# Solver sources
# =============================================================================
set(NEWTON_SOLVER_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/newton/newton.hpp)
set(CROUT_SOLVER_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/crout/crout.hpp)

# =============================================================================
# Copy necessary files to build directory
# =============================================================================
# Newton
file(GLOB NMODL_NEWTON_SOLVER_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/newton/*.h*")
file(COPY ${NMODL_NEWTON_SOLVER_HEADER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/include/newton/)
# Crout
file(GLOB NMODL_CROUT_SOLVER_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/crout/*.h*")
file(COPY ${NMODL_CROUT_SOLVER_HEADER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/include/crout/)
# Eigen
file(COPY ${NMODL_PROJECT_SOURCE_DIR}/ext/eigen/Eigen DESTINATION ${CMAKE_BINARY_DIR}/include/)

# =============================================================================
# Install solver headers and eigen from include
# =============================================================================
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/ DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}include)
