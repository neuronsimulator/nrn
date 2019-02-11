#=============================================================================
# pybind targets
#=============================================================================
set_source_files_properties( ${AUTO_GENERATED_FILES} PROPERTIES GENERATED TRUE)


set(PYNMODL_SOURCES
        ${PROJECT_SOURCE_DIR}/src/pybind/pyast.cpp
        ${PROJECT_SOURCE_DIR}/src/pybind/pyvisitor.cpp
        ${PROJECT_SOURCE_DIR}/src/pybind/pysymtab.cpp
        ${PROJECT_SOURCE_DIR}/src/pybind/pynmodl.cpp)

set(PYNMODL_HEADERS
        ${PROJECT_SOURCE_DIR}/src/ast/ast_decl.hpp
        ${PROJECT_SOURCE_DIR}/src/ast/ast_common.hpp
        ${PROJECT_SOURCE_DIR}/src/visitors/visitor.hpp
        ${PROJECT_SOURCE_DIR}/src/pybind/pybind_utils.hpp
        ${PROJECT_SOURCE_DIR}/src/pybind/pyast.hpp
        ${PROJECT_SOURCE_DIR}/src/pybind/pyvisitor.hpp)


pybind11_add_module(_nmodl ${PYNMODL_HEADERS} ${PYNMODL_SOURCES}
        $<TARGET_OBJECTS:symtab_obj>
        $<TARGET_OBJECTS:visitor_obj>
        $<TARGET_OBJECTS:lexer_obj>
        $<TARGET_OBJECTS:util_obj>
        $<TARGET_OBJECTS:printer_obj>
        )

add_dependencies(_nmodl pyastgen)
add_dependencies(_nmodl lexer_obj)
add_dependencies(_nmodl util_obj)

add_custom_command(TARGET _nmodl POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${PROJECT_SOURCE_DIR}/nmodl
                    ${CMAKE_BINARY_DIR}/nmodl
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:_nmodl>
                    ${CMAKE_BINARY_DIR}/nmodl
                   )

#=============================================================================
# Install python binding components
#=============================================================================
install(TARGETS _nmodl DESTINATION lib/python/nmodl)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/nmodl DESTINATION lib/python/)
