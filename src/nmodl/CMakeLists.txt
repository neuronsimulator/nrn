#=============================================================================
# NMODL sources
#=============================================================================
set(NMODL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arg_handler.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arg_handler.cpp
)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${NMODL_SOURCE_FILES}
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)

#=============================================================================
# Add executables
#=============================================================================
add_executable(nmodl
               ${NMODL_SOURCE_FILES})
target_link_libraries(nmodl printer codegen visitor symtab util lexer fmt)
