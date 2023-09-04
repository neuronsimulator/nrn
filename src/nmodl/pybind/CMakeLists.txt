# =============================================================================
# pybind targets
# =============================================================================
set_source_files_properties(${PYBIND_GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

# Set -fno-var-tracking-assignments on pyast.cpp with GCC to avoid a warning + double compilation
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  foreach(pybind_file "${PROJECT_BINARY_DIR}/src/pybind/pyast.cpp")
    get_source_file_property(pybind_file_compile_options "${pybind_file}" COMPILE_OPTIONS)
    if("${pybind_file_compile_options}" STREQUAL "NOTFOUND")
      set(pybind_file_compile_options)
    endif()
    list(APPEND pybind_file_compile_options "-fno-var-tracking-assignments")
    set_source_files_properties("${pybind_file}" PROPERTIES COMPILE_OPTIONS
                                                            "${pybind_file_compile_options}")
  endforeach()
endif()

# build nmodl python module under lib/nmodl
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/nmodl)

add_library(pyembed pyembed.cpp)
set_property(TARGET pyembed PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(pyembed PRIVATE util)

if(NOT LINK_AGAINST_PYTHON)
  add_library(pywrapper SHARED ${CMAKE_CURRENT_SOURCE_DIR}/wrapper.cpp)
  set_target_properties(pywrapper PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                             ${NMODL_PROJECT_BINARY_DIR}/lib)
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
    _nmodl NO_EXTRAS ${NMODL_PROJECT_SOURCE_DIR}/src/ast/ast_common.hpp
    ${NMODL_PROJECT_SOURCE_DIR}/src/pybind/pybind_utils.hpp
    ${NMODL_PROJECT_SOURCE_DIR}/src/pybind/pynmodl.cpp ${PYBIND_GENERATED_SOURCES})
  add_dependencies(_nmodl lexer pyastgen util)
  target_link_libraries(_nmodl PRIVATE printer symtab visitor pyembed)

  # in case of wheel, python module shouldn't link to wrapper library
  if(LINK_AGAINST_PYTHON)
    target_link_libraries(_nmodl PRIVATE pywrapper)
  endif()
endif()

# =============================================================================
# Copy python binding components and examples into build directory
# =============================================================================
file(
  GLOB NMODL_PYTHON_FILES
  RELATIVE "${NMODL_PROJECT_SOURCE_DIR}/nmodl/"
  CONFIGURE_DEPENDS "${NMODL_PROJECT_SOURCE_DIR}/nmodl/*.py")

foreach(file IN LISTS NMODL_PYTHON_FILES)
  cpp_cc_build_time_copy(INPUT ${NMODL_PROJECT_SOURCE_DIR}/nmodl/${file} OUTPUT
                         ${CMAKE_BINARY_DIR}/lib/nmodl/${file})
endforeach()
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
