include(${PROJECT_SOURCE_DIR}/cmake/nmodl/FlexHelper.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/nmodl/CompilerHelper.cmake)
include_directories(${NMODL_PROJECT_PLATLIB_SOURCE_DIR} ${NMODL_PROJECT_PLATLIB_BINARY_DIR})

list(APPEND NMODL_EXTRA_CXX_FLAGS ${NRN_SANITIZER_COMPILER_FLAGS})

# =============================================================================
# In case NRN_ENABLE_PYTHON=OFF, we cannot build Python bindings, so we unconditionally disable
# those
# =============================================================================
if(NOT NRN_ENABLE_PYTHON)
  set(NMODL_ENABLE_PYTHON_BINDINGS
      OFF
      CACHE BOOL "" FORCE)
endif()

# =============================================================================
# Create some random files.
# =============================================================================

# generate file with version number from git and nrnunits.lib file path
configure_file(${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/config/config.cpp.in
               ${NMODL_PROJECT_PLATLIB_BINARY_DIR}/config/config.cpp @ONLY)

# generate Doxyfile with correct source paths
configure_file(${PROJECT_SOURCE_DIR}/docs/nmodl/transpiler/Doxyfile.in
               ${PROJECT_BINARY_DIR}/docs/nmodl/transpiler/Doxyfile)

# =============================================================================
# Generate NMODL code, e.g. ast & visitor base classes.
# -----------------------------------------------------------------------------
if(NOT NMODL_ENABLE_PYTHON_BINDINGS)
  list(APPEND CODE_GENERATOR_OPTS "--disable-pybind")
endif()

# -----------------------------------------------------------------------------
# Part I: generate the list of generated files
# -----------------------------------------------------------------------------
execute_process(
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/language/code_generator.py
          ${CODE_GENERATOR_OPTS} --generate-cmake --base-dir ${CMAKE_CURRENT_BINARY_DIR}/language
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/language)

# Ensure that the above runs again, if its dependencies update.
set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/language/code_generator.py")

set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS
           ${CMAKE_CURRENT_SOURCE_DIR}/language/templates/code_generator.cmake)

# The list should be up-to-date now.
include(${CMAKE_CURRENT_BINARY_DIR}/language/code_generator.cmake)

# -----------------------------------------------------------------------------
# Part II: generate AST/Visitor classes from language definition
# -----------------------------------------------------------------------------
set_source_files_properties(${NMODL_GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

# Make the codegen options available as a dependency by storing them in a file.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/language/code_generator_opts.in
               ${CMAKE_CURRENT_BINARY_DIR}/language/code_generator_opts)

add_custom_command(
  OUTPUT ${NMODL_GENERATED_SOURCES}
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/language/code_generator.py
          ${CODE_GENERATOR_OPTS} --base-dir ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/language
  DEPENDS ${CODE_GENERATOR_PY_FILES}
  DEPENDS ${CODE_GENERATOR_YAML_FILES}
  DEPENDS ${CODE_GENERATOR_JINJA_FILES}
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/language/code_generator_opts
  COMMENT "-- NMODL : GENERATING AST CLASSES WITH PYTHON GENERATOR! --")
add_custom_target(nmodl_gen_sources DEPENDS ${NMODL_GENERATED_SOURCES})
unset(CODE_GENERATOR_OPTS)

# -----------------------------------------------------------------------------
# Target to propagate dependencies properly to lexer
# -----------------------------------------------------------------------------
add_custom_target(pyastgen DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ast/ast.cpp)

# =============================================================================
# Add extra compile flags to NMODL sources
# =============================================================================
add_compile_options(${NMODL_EXTRA_CXX_FLAGS})
if(NMODL_COMPILER_WARNING_SUPPRESSIONS)
  add_compile_options(${NMODL_COMPILER_WARNING_SUPPRESSIONS})
endif()
add_link_options(${NMODL_EXTRA_CXX_FLAGS})

add_subdirectory(codegen)
add_subdirectory(language)
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(printer)
add_subdirectory(symtab)
add_subdirectory(utils)
add_subdirectory(visitors)
add_subdirectory(pybind)
add_subdirectory(solver)

# =============================================================================
# NMODL sources
# =============================================================================
include_directories(${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})

# =============================================================================
# Add executables
# =============================================================================
add_executable(nmodl main.cpp)
target_link_libraries(
  nmodl
  CLI11::CLI11
  printer
  codegen
  visitor
  symtab
  util
  lexer
  ${NMODL_WRAPPER_LIBS})
add_dependencies(nmodl nmodl_copy_solver_files)
cpp_cc_configure_sanitizers(TARGET nmodl)

add_executable(nmodl::nmodl ALIAS nmodl)

# =============================================================================
# Add dependency with nmodl Python module (for consumer projects)
# =============================================================================
add_dependencies(nmodl pywrapper)

if(NMODL_ENABLE_PYTHON_BINDINGS)
  add_subdirectory(bindings)
  add_dependencies(nmodl nmodl_copy_python_files ${NMODL_PYTHON_TARGETS})
endif()

# =============================================================================
# Install executable
# =============================================================================
set(NMODL_INSTALL_DIR_SUFFIX ${NRN_INSTALL_DATA_PREFIX})

install(
  TARGETS nmodl
  EXPORT nmodlTargets
  RUNTIME DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}bin)

set(nrnunits_start "")
set(nrnunits_stop "")
configure_file(${PROJECT_SOURCE_DIR}/share/nmodl/nrnunits.lib.in
               ${CMAKE_CURRENT_BINARY_DIR}/share/nmodl/nrnunits.lib @ONLY)
set(nrnunits_start "R\"nrnunits(")
set(nrnunits_stop ")nrnunits\"")
configure_file(${PROJECT_SOURCE_DIR}/share/nmodl/nrnunits.lib.in
               ${PROJECT_BINARY_DIR}/share/nmodl/embedded_nrnunits.lib @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/share/nmodl/nrnunits.lib
        DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}share/nmodl)

install(
  EXPORT nmodlTargets
  FILE nmodlTargets.cmake
  NAMESPACE nmodl::
  DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}lib/cmake/nmodl)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/nmodl/Config.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/nmodlConfig.cmake"
  INSTALL_DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}lib/cmake/nmodl)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/nmodlConfigVersion.cmake"
                                 COMPATIBILITY AnyNewerVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nmodlConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/nmodlConfigVersion.cmake"
        DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}lib/cmake/nmodl)

# =============================================================================
# Build status
# =============================================================================
message(STATUS "")
message(STATUS "Configured NMODL")
message(STATUS "")
message(STATUS "-- NMODL is enabled with following build configuration:")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS " Build option       | Status")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS "CXX COMPILER        | ${CMAKE_CXX_COMPILER}")
message(STATUS "COMPILE FLAGS       | ${COMPILER_FLAGS}")
message(STATUS "EXTRA FLAGS         | ${NMODL_EXTRA_CXX_FLAGS}")
message(STATUS "Build Type          | ${CMAKE_BUILD_TYPE}")
message(STATUS "Python Bindings     | ${NMODL_ENABLE_PYTHON_BINDINGS}")
message(STATUS "Flex                | ${FLEX_EXECUTABLE}")
message(STATUS "Bison               | ${BISON_EXECUTABLE}")
message(STATUS "Python              | ${PYTHON_EXECUTABLE}")
message(STATUS "  Linked against    | ${NRN_LINK_AGAINST_PYTHON}")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS "")
