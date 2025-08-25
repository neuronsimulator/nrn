# =============================================================================
# Add extra compile flags to NMODL test sources
# =============================================================================
include(${PROJECT_SOURCE_DIR}/cmake/nmodl/FlexHelper.cmake)
add_compile_options(${NMODL_EXTRA_CXX_FLAGS})
add_link_options(${NMODL_EXTRA_CXX_FLAGS})
if(NMODL_TESTS_COMPILER_WARNING_SUPPRESSIONS)
  add_compile_options(${NMODL_TESTS_COMPILER_WARNING_SUPPRESSIONS})
endif()

add_library(nmodl_test_flags INTERFACE)

target_include_directories(nmodl_test_flags INTERFACE ${PYBIND11_INCLUDE_DIR}
                                                      ${PYTHON_INCLUDE_DIRS})
target_include_directories(nmodl_test_flags INTERFACE ${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/solver)
target_include_directories(nmodl_test_flags INTERFACE ${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/utils)
target_include_directories(nmodl_test_flags INTERFACE ${NMODL_PROJECT_PLATLIB_SOURCE_DIR})
target_include_directories(nmodl_test_flags INTERFACE ${NMODL_PROJECT_PLATLIB_BINARY_DIR})
target_include_directories(nmodl_test_flags INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nmodl_test_flags INTERFACE ${PROJECT_SOURCE_DIR}/external/eigen)

# =============================================================================
# Common input data library
# =============================================================================
add_library(test_util STATIC utils/nmodl_constructs.cpp utils/test_utils.cpp)
target_link_libraries(test_util PUBLIC spdlog::spdlog nmodl_test_flags)

# =============================================================================
# Common input data library
# =============================================================================
add_library(config STATIC ${PROJECT_BINARY_DIR}/src/nmodl/config/config.cpp)
target_link_libraries(config PUBLIC nmodl_test_flags)

# =============================================================================
# Test executables
# =============================================================================
add_executable(testutils utils/string_utils.cpp)
add_executable(testmodtoken modtoken/modtoken.cpp)
add_executable(testlexer lexer/tokens.cpp)
add_executable(testparser parser/parser.cpp)
add_executable(
  testvisitor
  visitor/main.cpp
  visitor/after_cvode_to_cnexp.cpp
  visitor/constant_folder.cpp
  visitor/external.cpp
  visitor/defuse_analyze.cpp
  visitor/global_to_range.cpp
  visitor/implicit_argument.cpp
  visitor/inline.cpp
  visitor/json.cpp
  visitor/kinetic_block.cpp
  visitor/localize.cpp
  visitor/localrename.cpp
  visitor/cvode.cpp
  visitor/local_to_assigned.cpp
  visitor/lookup.cpp
  visitor/loop_unroll.cpp
  visitor/misc.cpp
  visitor/neuron_solve.cpp
  visitor/nmodl.cpp
  visitor/perf.cpp
  visitor/rename.cpp
  visitor/rename_function_arguments.cpp
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
add_executable(testcrout crout/crout.cpp ${CROUT_SOLVER_SOURCE_FILES})
add_executable(testunitlexer units/lexer.cpp)
add_executable(testunitparser units/parser.cpp)
add_executable(
  testcodegen
  codegen/main.cpp
  codegen/codegen_info.cpp
  codegen/codegen_helper.cpp
  codegen/codegen_utils.cpp
  codegen/codegen_coreneuron_cpp_visitor.cpp
  codegen/codegen_neuron_cpp_visitor.cpp
  codegen/transform.cpp
  codegen/codegen_compatibility_visitor.cpp)

target_link_libraries(testutils PRIVATE lexer util nmodl_test_flags)
target_link_libraries(testmodtoken PRIVATE lexer util nmodl_test_flags)
target_link_libraries(testlexer PRIVATE lexer util nmodl_test_flags)
target_link_libraries(
  testparser
  PRIVATE visitor
          symtab
          lexer
          util
          test_util
          printer
          ${NMODL_WRAPPER_LIBS}
          nmodl_test_flags)
target_link_libraries(
  testvisitor
  PRIVATE visitor
          symtab
          lexer
          util
          test_util
          printer
          codegen
          ${NMODL_WRAPPER_LIBS}
          nmodl_test_flags)
target_link_libraries(
  testcodegen
  PRIVATE codegen
          visitor
          symtab
          lexer
          util
          test_util
          printer
          ${NMODL_WRAPPER_LIBS}
          nmodl_test_flags)
target_link_libraries(testprinter PRIVATE printer util nmodl_test_flags)
target_link_libraries(testsymtab PRIVATE symtab lexer util nmodl_test_flags)
target_link_libraries(testunitlexer PRIVATE lexer util nmodl_test_flags)
target_link_libraries(testunitparser PRIVATE lexer test_util config nmodl_test_flags)

# =============================================================================
# Use catch_discover instead of add_test for granular test result reporting.
# =============================================================================
set(test_env ${NRN_SANITIZER_ENABLE_ENVIRONMENT})
set(testvisitor_env "PYTHONPATH=${PROJECT_BINARY_DIR}/lib:$ENV{PYTHONPATH}")
if(NOT NRN_LINK_AGAINST_PYTHON)
  list(APPEND testvisitor_env "NMODL_PYLIB=$ENV{NMODL_PYLIB}")
endif()

# Without main from Catch2
target_link_libraries(testcodegen PRIVATE Catch2::Catch2 nmodl_test_flags)
target_link_libraries(testvisitor PRIVATE Catch2::Catch2 nmodl_test_flags)

# With main from Catch2
target_link_libraries(testutils PRIVATE Catch2::Catch2WithMain nmodl_test_flags)
target_link_libraries(testmodtoken PRIVATE Catch2::Catch2WithMain nmodl_test_flags)
target_link_libraries(testlexer PRIVATE Catch2::Catch2WithMain nmodl_test_flags)
target_link_libraries(testparser PRIVATE Catch2::Catch2WithMain nmodl_test_flags)
target_link_libraries(testprinter PRIVATE Catch2::Catch2WithMain nmodl_test_flags)
target_link_libraries(testsymtab PRIVATE Catch2::Catch2WithMain nmodl_test_flags)
target_link_libraries(testnewton PRIVATE Catch2::Catch2WithMain nmodl_test_flags)
target_link_libraries(testcrout PRIVATE Catch2::Catch2WithMain nmodl_test_flags)
target_link_libraries(testunitlexer PRIVATE Catch2::Catch2WithMain nmodl_test_flags)
target_link_libraries(testunitparser PRIVATE Catch2::Catch2WithMain nmodl_test_flags)

foreach(
  test_name
  testutils
  testmodtoken
  testlexer
  testparser
  testprinter
  testsymtab
  testnewton
  testcrout
  testunitlexer
  testunitparser)

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

foreach(test_name testcodegen testvisitor)
  if(NOT NRN_LINK_AGAINST_PYTHON)
    target_link_libraries(${test_name} PRIVATE pywrapper ${PYTHON_LIBRARY})
  endif()
  cpp_cc_configure_sanitizers(TARGET ${test_name})
  set(env ${test_env})
  list(APPEND env ${${test_name}_env})
  string(REPLACE ";" ";ENVIRONMENT;" _environment_vars_list "${env}")
  catch_discover_tests(${test_name} TEST_PREFIX "${test_name}/" PROPERTIES ENVIRONMENT
                       "${_environment_vars_list}")
endforeach()

# Generate include files used in test input, see
# test/nmodl/transpiler/unit/utils/nmodl_constructs.cpp
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Unit.inc" "UNITSON \n UNITSOFF \n UNITSON \n UNITSOFF")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/included.file" "TITLE")

if(WIN32)
  set(NMODL_TEST_PYTHONPATH "${PROJECT_BINARY_DIR}/lib/python;$ENV{PYTHONPATH}")
else()
  set(NMODL_TEST_PYTHONPATH "${PROJECT_BINARY_DIR}/lib/python:$ENV{PYTHONPATH}")
endif()

# =============================================================================
# pybind11 tests
# =============================================================================
if(NRN_ENABLE_PYTHON)
  if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND NRN_SANITIZERS))
    # Most of the Python tests added by `add_nrn_test` run in their own directories. Unfortunately,
    # pytest-cov does not seem to have an option to specify the output file (it _always_ outputs to
    # `coverage.xml`), which means that the various tests here may clobber each other's coverage
    # reports. The way out of this is to a) either specify `WORKING_DIRECTORY` of each test, or b)
    # use a coverage config file (see
    # https://coverage.readthedocs.io/en/latest/config.html#xml-output); the latter requires far
    # more work than the former, so we use option a) in the below.
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ode)
    add_test(
      NAME Ode
      COMMAND ${PYTHON_EXECUTABLE} ${NRN_PYTEST_LAUNCHER} ${CMAKE_CURRENT_SOURCE_DIR}/ode
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ode)
    set_tests_properties(
      Ode
      PROPERTIES ENVIRONMENT
                 "PYTHONPATH=${NMODL_TEST_PYTHONPATH};NEURONHOME=${PROJECT_BINARY_DIR}/share/nrn")
    cpp_cc_configure_sanitizers(TEST Ode PRELOAD)
  endif()

  if(NMODL_ENABLE_PYTHON_BINDINGS)
    # Apple Clang and ASAN do not play along nicely with NMODL's Python bindings, so we skip these
    # tests
    if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND NRN_SANITIZERS))
      file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pybind)
      add_test(
        NAME Pybind
        COMMAND ${PYTHON_EXECUTABLE} ${NRN_PYTEST_LAUNCHER} ${CMAKE_CURRENT_SOURCE_DIR}/pybind
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pybind)
      set_tests_properties(
        Pybind
        PROPERTIES
          ENVIRONMENT
          "PYTHONPATH=${NMODL_TEST_PYTHONPATH};NMODLHOME=${PROJECT_BINARY_DIR};NEURONHOME=${PROJECT_BINARY_DIR}/share/nrn"
      )
      cpp_cc_configure_sanitizers(TEST Pybind PRELOAD)
    endif()
  endif()
endif()
