# =============================================================================
# NMODL sources
# =============================================================================
include_directories(${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})
set(NMODL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arg_handler.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arg_handler.cpp)

# =============================================================================
# Add executables
# =============================================================================
add_executable(nmodl ${NMODL_SOURCE_FILES})
target_link_libraries(nmodl printer codegen visitor symtab util lexer)

# =============================================================================
# Install executable
# =============================================================================
install(TARGETS nmodl DESTINATION bin)
