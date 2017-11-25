set(VISITOR_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/src/visitors/astvisitor.cpp
    ${PROJECT_SOURCE_DIR}/src/visitors/verbatim_visitor.cpp
)

set_source_files_properties(
    ${VISITOR_SOURCE_FILES}
    PROPERTIES GENERATED TRUE
)

add_library(visitor
            STATIC
            ${VISITOR_SOURCE_FILES})

# visitors can not be compiled without parser files
# because modtoken does use location, position etc.
add_dependencies(visitor lexer)

add_executable(nocmodl_visitor main.cpp)
target_link_libraries(nocmodl_visitor lexer visitor)