#=============================================================================
# Codegen sources
#=============================================================================
set(CODEGEN_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_info.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/codegen_base_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/codegen_base_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/codegen_helper_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/codegen_helper_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/c/codegen_c_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/c/codegen_c_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/c-openmp/codegen_c_omp_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/c-openmp/codegen_c_omp_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/c-openacc/codegen_c_acc_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/c-openacc/codegen_c_acc_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/c-cuda/codegen_c_cuda_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/c-cuda/codegen_c_cuda_visitor.cpp
)

#=============================================================================
# Codegen library and executable
#=============================================================================
add_library(codegen
            STATIC
            ${CODEGEN_SOURCE_FILES})

add_dependencies(codegen lexer util)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CODEGEN_SOURCE_FILES}
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)
