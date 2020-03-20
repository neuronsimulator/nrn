# =============================================================================
# NMODL sources
# =============================================================================
include_directories(${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})
set(NMODL_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

# =============================================================================
# Add executables
# =============================================================================
add_executable(nmodl ${NMODL_SOURCE_FILES})
target_link_libraries(nmodl printer codegen visitor symtab util lexer)

# =============================================================================
# Add dependency with nmodl pytnon module (for consumer projects)
# =============================================================================
add_dependencies(nmodl _nmodl)

# =============================================================================
# Install executable
# =============================================================================
install(TARGETS nmodl DESTINATION bin)
install(FILES nmodl.hpp DESTINATION include)
