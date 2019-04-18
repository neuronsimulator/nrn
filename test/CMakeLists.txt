set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/ext/catch ${PROJECT_SOURCE_DIR}/test)
include_directories(${PROJECT_SOURCE_DIR}/src/solver)
include_directories(${PROJECT_SOURCE_DIR}/ext/eigen)

# =============================================================================
# Common input data library
# =============================================================================
add_library(test_util
            STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/utils/nmodl_constructs.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_utils.cpp)

# =============================================================================
# Common input data library
# =============================================================================
add_library(config STATIC ${PROJECT_BINARY_DIR}/config.cpp)

# =============================================================================
# Test executables
# =============================================================================
add_executable(testmodtoken modtoken/modtoken.cpp)
add_executable(testlexer lexer/tokens.cpp)
add_executable(testparser parser/parser.cpp)
add_executable(testvisitor visitor/visitor.cpp)
add_executable(testprinter printer/printer.cpp)
add_executable(testsymtab symtab/symbol_table.cpp)
add_executable(testnewton newton/newton.cpp ${SOLVER_SOURCE_FILES})
add_executable(testunitlexer units/lexer.cpp)
add_executable(testunitparser units/parser.cpp)

target_link_libraries(testmodtoken lexer)
target_link_libraries(testlexer lexer)
target_link_libraries(testparser lexer test_util)
target_link_libraries(testvisitor symtab visitor lexer util test_util printer)
target_link_libraries(testprinter printer)
target_link_libraries(testsymtab symtab lexer util)
target_link_libraries(testunitlexer lexer util)
target_link_libraries(testunitparser lexer test_util config)

add_test(NAME ModToken COMMAND testmodtoken)
add_test(NAME Lexer COMMAND testlexer)
add_test(NAME Parser COMMAND testparser)
add_test(NAME Visitor COMMAND testvisitor)
add_test(NAME Printer COMMAND testprinter)
add_test(NAME Symtab COMMAND testsymtab)
add_test(NAME Newton COMMAND testnewton)
add_test(NAME UnitLexer COMMAND testunitlexer)
add_test(NAME UnitParser COMMAND testunitparser)

set_tests_properties(Visitor PROPERTIES ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}:$ENV{PYTHONPATH})

# =============================================================================
# pybind11 tests
# =============================================================================

add_test(NAME Ode
         COMMAND python3 -m pytest ${PROJECT_SOURCE_DIR}/test/ode)
set_tests_properties(Ode PROPERTIES ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}:$ENV{PYTHONPATH})
add_test(NAME Pybind
         COMMAND python3 -m pytest ${PROJECT_SOURCE_DIR}/test/pybind)
set_tests_properties(Pybind PROPERTIES ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}:$ENV{PYTHONPATH})
