set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# =============================================================================
# Add extra compile flags to NMODL test sources
# =============================================================================
add_compile_options(${NMODL_EXTRA_CXX_FLAGS})
add_link_options(${NMODL_EXTRA_CXX_FLAGS})
add_compile_options(${NMODL_TESTS_COMPILER_WARNING_SUPPRESSIONS})

include_directories(${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})
include_directories(${NMODL_PROJECT_SOURCE_DIR}/test)
include_directories(${NMODL_PROJECT_SOURCE_DIR}/src/solver)
include_directories(${NMODL_PROJECT_SOURCE_DIR}/src/utils)
include_directories(${NMODL_PROJECT_SOURCE_DIR}/ext/eigen)

# =============================================================================
# Common input data library
# =============================================================================
add_library(test_util STATIC utils/nmodl_constructs.cpp utils/test_utils.cpp)

# =============================================================================
# Common input data library
# =============================================================================
add_library(config STATIC ${PROJECT_BINARY_DIR}/src/config/config.cpp)

# =============================================================================
# Test executables
# =============================================================================
add_executable(testmodtoken modtoken/modtoken.cpp)
add_executable(testlexer lexer/tokens.cpp)
add_executable(testparser parser/parser.cpp)
add_executable(
  testvisitor
  visitor/main.cpp
  visitor/after_cvode_to_cnexp.cpp
  visitor/constant_folder.cpp
  visitor/defuse_analyze.cpp
  visitor/global_to_range.cpp
  visitor/implicit_argument.cpp
  visitor/inline.cpp
  visitor/ispc_rename.cpp
  visitor/json.cpp
  visitor/kinetic_block.cpp
  visitor/localize.cpp
  visitor/local_to_assigned.cpp
  visitor/lookup.cpp
  visitor/loop_unroll.cpp
  visitor/misc.cpp
  visitor/neuron_solve.cpp
  visitor/nmodl.cpp
  visitor/perf.cpp
  visitor/rename.cpp
  visitor/semantic_analysis.cpp
  visitor/solve_block.cpp
  visitor/steadystate.cpp
  visitor/sympy_conductance.cpp
  visitor/sympy_solver.cpp
  visitor/units.cpp
  visitor/var_usage.cpp
  visitor/verbatim.cpp
  visitor/node_index.cpp)
add_executable(testprinter printer/printer.cpp)
add_executable(testsymtab symtab/symbol_table.cpp)
add_executable(testnewton newton/newton.cpp ${NEWTON_SOLVER_SOURCE_FILES})
add_executable(testfast_math fast_math/fast_math.cpp ${NEWTON_SOLVER_SOURCE_FILES})
add_executable(testunitlexer units/lexer.cpp)
add_executable(testunitparser units/parser.cpp)
add_executable(
  testcodegen codegen/main.cpp codegen/codegen_ispc.cpp codegen/codegen_helper.cpp
              codegen/codegen_utils.cpp codegen/codegen_c_visitor.cpp codegen/transform.cpp)

target_link_libraries(testmodtoken lexer util)
target_link_libraries(testlexer lexer util)
target_link_libraries(
  testparser
  visitor
  symtab
  lexer
  util
  test_util
  printer
  ${NMODL_WRAPPER_LIBS})
target_link_libraries(
  testvisitor
  visitor
  symtab
  lexer
  util
  test_util
  printer
  ${NMODL_WRAPPER_LIBS})
target_link_libraries(
  testcodegen
  codegen
  visitor
  symtab
  lexer
  util
  test_util
  printer
  ${NMODL_WRAPPER_LIBS})
target_link_libraries(testprinter printer util)
target_link_libraries(testsymtab symtab lexer util)
target_link_libraries(testunitlexer lexer util)
target_link_libraries(testunitparser lexer test_util config)

# =============================================================================
# Use catch_discover instead of add_test for granular test result reporting.
# =============================================================================
set(test_env ${NMODL_SANITIZER_ENABLE_ENVIRONMENT})
set(testvisitor_env "PYTHONPATH=${PROJECT_BINARY_DIR}/lib:$ENV{PYTHONPATH}")
if(NOT LINK_AGAINST_PYTHON)
  list(APPEND testvisitor_env "NMODL_PYLIB=$ENV{NMODL_PYLIB}")
  list(APPEND testvisitor_env
       "NMODL_WRAPLIB=${PROJECT_BINARY_DIR}/lib/nmodl/libpywrapper${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

foreach(
  test_name
  testcodegen
  testmodtoken
  testlexer
  testparser
  testvisitor
  testprinter
  testsymtab
  testnewton
  testfast_math
  testunitlexer
  testunitparser)

  target_link_libraries(${test_name} Catch2::Catch2)
  cpp_cc_configure_sanitizers(TARGET ${test_name})
  set(env ${test_env})
  list(APPEND env ${${test_name}_env})
  # See https://github.com/catchorg/Catch2/issues/2424
  string(REPLACE ";" ";ENVIRONMENT;" _environment_vars_list "${env}")
  # Catch2 doesn't set these environment variables when running the executable to discover tests
  # (https://github.com/catchorg/Catch2/issues/1810). This means that in builds that enable it, we
  # see errors from the LeakSanitizer during the build. These seem to be harmless because Catch2
  # does not seem to check the exit code.
  catch_discover_tests(${test_name} TEST_PREFIX "${test_name}/" PROPERTIES ENVIRONMENT
                       "${_environment_vars_list}")
endforeach()

# =============================================================================
# pybind11 tests
# =============================================================================
add_test(NAME Ode COMMAND ${PYTHON_EXECUTABLE} -m pytest ${NMODL_PROJECT_SOURCE_DIR}/test/unit/ode)
set_tests_properties(Ode PROPERTIES ENVIRONMENT
                                    PYTHONPATH=${PROJECT_BINARY_DIR}/lib:$ENV{PYTHONPATH})

if(NMODL_ENABLE_PYTHON_BINDINGS)
  add_test(NAME Pybind COMMAND ${PYTHON_EXECUTABLE} -m pytest
                               ${NMODL_PROJECT_SOURCE_DIR}/test/unit/pybind)
  set_tests_properties(Pybind PROPERTIES ENVIRONMENT
                                         PYTHONPATH=${PROJECT_BINARY_DIR}/lib:$ENV{PYTHONPATH})
  cpp_cc_configure_sanitizers(TEST Pybind PRELOAD)
endif()
