# =============================================================================
# Codegen sources
# =============================================================================
set(CODEGEN_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_acc_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_acc_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_compatibility_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_compatibility_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_cuda_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_cuda_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_omp_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_omp_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_c_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_c_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_helper_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_helper_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_info.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_ispc_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_ispc_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_naming.hpp)

# =============================================================================
# Codegen library and executable
# =============================================================================
add_library(codegen STATIC ${CODEGEN_SOURCE_FILES})

add_dependencies(codegen lexer util visitor)

# =============================================================================
# Install include files
# =============================================================================
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/fast_math.ispc DESTINATION include/nmodl)
