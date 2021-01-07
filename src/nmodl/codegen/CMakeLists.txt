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
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_naming.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_utils.hpp)

# =============================================================================
# Codegen library and executable
# =============================================================================
add_library(codegen STATIC ${CODEGEN_SOURCE_FILES})

add_dependencies(codegen lexer util visitor)

# copy to build directory to make usable from build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fast_math.ispc
               ${CMAKE_BINARY_DIR}/include/nmodl/fast_math.ispc COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fast_math.hpp
               ${CMAKE_BINARY_DIR}/include/nmodl/fast_math.hpp COPYONLY)

# =============================================================================
# Install include files
# =============================================================================
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/fast_math.ispc
        DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}include/nmodl)
