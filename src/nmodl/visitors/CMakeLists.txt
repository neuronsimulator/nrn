#=============================================================================
# Visitor sources
#=============================================================================
set(VISITOR_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/ast_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cnexp_solve_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cnexp_solve_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/defuse_analyze_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/defuse_analyze_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/differential_equation_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/differential_equation_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/inline_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/inline_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/json_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/local_var_rename_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/local_var_rename_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/localize_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/localize_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lookup_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lookup_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nmodl_visitor_helper.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perf_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perf_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rename_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rename_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symtab_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symtab_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/symtab_visitor_helper.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/var_usage_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/var_usage_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_var_rename_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_var_rename_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verbatim_visitor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/visitor_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/visitor_utils.hpp
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
set_property(TARGET visitor PROPERTY POSITION_INDEPENDENT_CODE ON)

add_dependencies(visitor lexer util)

add_executable(nmodl_visitor main.cpp)
target_link_libraries(nmodl_visitor printer visitor symtab util lexer)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${VISITOR_SOURCE_FILES}
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)
