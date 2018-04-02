#=============================================================================
# Parser executable
#=============================================================================

# lexer library links with all parser related files
# so no eed to have parser as a separate library

add_executable(nocmodl_parser main_nmodl.cpp)
add_executable(c_parser main_c.cpp)

target_link_libraries(nocmodl_parser lexer)
target_link_libraries(c_parser lexer)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${CMAKE_CURRENT_SOURCE_DIR}/main_nmodl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_context.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/diffeq_context.hpp
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)