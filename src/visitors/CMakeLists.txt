#=============================================================================
# Visitor sources
#=============================================================================
set(VISITOR_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/ast_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/json_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_visitor.cpp
)

set_source_files_properties(
    ${VISITOR_SOURCE_FILES}
    PROPERTIES GENERATED TRUE
)

#=============================================================================
# Visitor library and executable
#=============================================================================
add_library(visitor
            STATIC
            ${VISITOR_SOURCE_FILES})

add_dependencies(visitor lexer)

add_executable(nocmodl_visitor main.cpp)
target_link_libraries(nocmodl_visitor lexer printer visitor)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${VISITOR_SOURCE_FILES}
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)