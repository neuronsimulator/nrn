#=============================================================================
# Visitor sources
#=============================================================================
set(VISITOR_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/visitors/ast_visitor.cpp
    ${PROJECT_SOURCE_DIR}/src/visitors/json_visitor.cpp
    ${PROJECT_SOURCE_DIR}/src/visitors/verbatim_visitor.cpp
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