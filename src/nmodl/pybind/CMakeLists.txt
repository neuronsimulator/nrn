# =============================================================================
# pybind targets
# =============================================================================
set_source_files_properties(${PYBIND_GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

# build nmodl python module under lib/nmodl
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/nmodl)

foreach(file ast.py dsl.py ode.py symtab.py visitor.py __init__.py)
  list(APPEND NMODL_PYTHON_FILES_IN ${NMODL_PROJECT_SOURCE_DIR}/nmodl/${file})
  list(APPEND NMODL_PYTHON_FILES_OUT ${PROJECT_BINARY_DIR}/lib/nmodl/${file})
endforeach()

add_library(pyembed ${CMAKE_CURRENT_SOURCE_DIR}/pyembed.cpp)
set_property(TARGET pyembed PROPERTY POSITION_INDEPENDENT_CODE ON)

if(NOT LINK_AGAINST_PYTHON)
  add_library(pywrapper SHARED ${CMAKE_CURRENT_SOURCE_DIR}/wrapper.cpp)
  set_target_properties(pywrapper PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
else()
  add_library(pywrapper ${CMAKE_CURRENT_SOURCE_DIR}/wrapper.cpp)
  set_property(TARGET pywrapper PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_compile_definitions(pyembed PRIVATE NMODL_STATIC_PYWRAPPER=1)
endif()

target_include_directories(pyembed PRIVATE ${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})
target_include_directories(pywrapper PRIVATE ${pybind11_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})

# ~~~
# pybind11::embed adds PYTHON_LIBRARIES to target_link_libraries. To avoid link to
# libpython, we can use `module` interface library from pybind11.
# ~~~
if(NOT LINK_AGAINST_PYTHON)
  target_link_libraries(pywrapper PRIVATE pybind11::module)
else()
  target_link_libraries(pywrapper PRIVATE pybind11::embed)
endif()

# avoid _nmodl target if python bindings are disabled
if(NMODL_ENABLE_PYTHON_BINDINGS)
  # ~~~
  # Note that LTO causes link time errors with GCC 8. To avoid this, we disable LTO
  # for pybind using NO_EXTRAS. See #266.
  # ~~~
  pybind11_add_module(
    _nmodl
    NO_EXTRAS
    ${NMODL_PROJECT_SOURCE_DIR}/src/ast/ast_common.hpp
    ${NMODL_PROJECT_SOURCE_DIR}/src/pybind/pybind_utils.hpp
    ${NMODL_PROJECT_SOURCE_DIR}/src/pybind/pynmodl.cpp
    ${PYBIND_GENERATED_SOURCES}
    $<TARGET_OBJECTS:symtab_obj>
    $<TARGET_OBJECTS:visitor_obj>
    $<TARGET_OBJECTS:lexer_obj>
    $<TARGET_OBJECTS:util_obj>
    $<TARGET_OBJECTS:printer_obj>)

  add_dependencies(_nmodl pyastgen lexer_obj util_obj)
  target_link_libraries(_nmodl PRIVATE fmt::fmt pyembed)

  # in case of wheel, python module shouldn't link to wrapper library
  if(LINK_AGAINST_PYTHON)
    target_link_libraries(_nmodl PRIVATE pywrapper)
  endif()

  add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/lib/nmodl
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:_nmodl> ${PROJECT_BINARY_DIR}/nmodl
    DEPENDS ${NMODL_PYTHON_FILES_IN} _nmodl
    COMMENT "-- COPYING NMODL PYTHON MODULE --")
endif()

add_custom_target(copy_python_files ALL DEPENDS ${NMODL_PYTHON_FILES_OUT})
add_custom_command(
  OUTPUT ${NMODL_PYTHON_FILES_OUT}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${NMODL_PROJECT_SOURCE_DIR}/nmodl
          ${PROJECT_BINARY_DIR}/lib/nmodl
  DEPENDS ${NMODL_PYTHON_FILES_IN}
  COMMENT "-- COPYING NMODL PYTHON FILES --")

# =============================================================================
# Copy python binding components and examples into build directory
# =============================================================================
file(GLOB NMODL_PYTHON_HELPER_FILES "${NMODL_PROJECT_SOURCE_DIR}/nmodl/*.py")
file(COPY ${NMODL_PYTHON_HELPER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/lib/nmodl/)
file(COPY ${NMODL_PROJECT_SOURCE_DIR}/nmodl/ext DESTINATION ${CMAKE_BINARY_DIR}/lib/nmodl/)

# =============================================================================
# Install python binding components
# =============================================================================
# ~~~
# scikit already installs the package in /nmodl. If we add it another time
# things are installed twice with the wheel and in weird places. Let's just
# move the .so libs
# ~~~
if(NOT LINK_AGAINST_PYTHON)
  install(TARGETS pywrapper DESTINATION ${NMODL_INSTALL_DIR_SUFFIX}lib)
  if(NMODL_ENABLE_PYTHON_BINDINGS)
    install(TARGETS _nmodl DESTINATION nmodl/)
  endif()
elseif(SKBUILD)
  # skbuild needs the installation dir to be in nmodl to do the correct inplace
  install(
    DIRECTORY ${CMAKE_BINARY_DIR}/lib/nmodl
    DESTINATION .
    PATTERN "__pycache__" EXCLUDE)
else()
  install(
    DIRECTORY ${CMAKE_BINARY_DIR}/lib/
    DESTINATION lib
    PATTERN "__pycache__" EXCLUDE)
endif()
