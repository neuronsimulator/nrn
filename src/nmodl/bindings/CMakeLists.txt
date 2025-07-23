# =============================================================================
# NMODL Python bindings (_not_ the embedded interpreter!)
# =============================================================================

include("${PROJECT_SOURCE_DIR}/cmake/PythonCompileHelper.cmake")

set(NMODL_PYTHON_TARGETS nmodl_py)

set(nmodl_default_python_target ${NMODL_PYTHON_TARGETS})

set_source_files_properties(${PYBIND_GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

# Set -fno-var-tracking-assignments on pyast.cpp with GCC to avoid a warning + double compilation
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  foreach(pybind_file "${NMODL_PROJECT_PLATLIB_BINARY_DIR}/pybind/pyast.cpp")
    get_source_file_property(pybind_file_compile_options "${pybind_file}" COMPILE_OPTIONS)
    if("${pybind_file_compile_options}" STREQUAL "NOTFOUND")
      set(pybind_file_compile_options)
    endif()
    list(APPEND pybind_file_compile_options "-fno-var-tracking-assignments")
    set_source_files_properties("${pybind_file}" PROPERTIES COMPILE_OPTIONS
                                                            "${pybind_file_compile_options}")
  endforeach()
endif()

# TODO handle case where NRN_ENABLE_PYTHON_DYNAMIC has multiple entries
if(NRN_ENABLE_PYTHON_DYNAMIC)
  set(python_includes "${PYTHON_INCLUDE_DIRS}")
  set(python_library "${PYTHON_LIBRARY}")
else()
  set(python_includes "${NRN_DEFAULT_PYTHON_INCLUDES}")
  set(python_library "${NRN_DEFAULT_PYTHON_LIBRARIES}")
endif()

add_nrn_python_library(
  _nmodl
  TARGET
  ${nmodl_default_python_target}
  PYTHON_VERSION
  ${NRN_DEFAULT_PYTHON_VERSION}
  LANGUAGE
  CXX
  SOURCES
  pynmodl.cpp
  ${PYBIND_GENERATED_SOURCES}
  INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/../pybind
  ${PYBIND11_INCLUDE_DIR}
  ${python_includes}
  OUTPUT_DIR
  ${PROJECT_BINARY_DIR}/${NRN_INSTALL_PYTHON_PREFIX}/nmodl)
add_dependencies(${nmodl_default_python_target} lexer pyastgen util)
target_link_libraries(${nmodl_default_python_target} PRIVATE printer symtab visitor pyembed)
set_target_properties(${nmodl_default_python_target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG
                                                                ${NMODL_PROJECT_PURELIB_BINARY_DIR})
set_target_properties(${nmodl_default_python_target} PROPERTIES CXX_VISIBILITY_PRESET hidden)

if(MSVC)
  target_compile_options(${nmodl_default_python_target} PRIVATE /bigobj)
endif()

# in case of wheel, python module shouldn't link to wrapper library
if(NRN_LINK_AGAINST_PYTHON)
  target_link_libraries(${nmodl_default_python_target} PRIVATE pywrapper)
endif()

# =============================================================================
# Copy python binding components and examples into build directory
# =============================================================================

file(
  GLOB NMODL_PYTHON_FILES
  RELATIVE "${NMODL_PROJECT_PURELIB_SOURCE_DIR}/"
  CONFIGURE_DEPENDS "${NMODL_PROJECT_PURELIB_SOURCE_DIR}/*.py")

foreach(file IN LISTS NMODL_PYTHON_FILES)
  cpp_cc_build_time_copy(
    INPUT ${NMODL_PROJECT_PURELIB_SOURCE_DIR}/${file}
    OUTPUT ${NMODL_PROJECT_PURELIB_BINARY_DIR}/${file}
    NO_TARGET)
  list(APPEND nmodl_python_binary_dir_files "${NMODL_PROJECT_PURELIB_BINARY_DIR}/${file}")
endforeach()

add_custom_target(nmodl_copy_python_files ALL DEPENDS ${nmodl_python_binary_dir_files})
cpp_cc_build_time_copy(
  INPUT ${NMODL_PROJECT_PURELIB_SOURCE_DIR}/ext
  OUTPUT ${NMODL_PROJECT_PURELIB_BINARY_DIR}
  NO_TARGET)

# =============================================================================
# Install targets
# =============================================================================

foreach(nmodl_python_target ${NMODL_PYTHON_TARGETS})
  install(TARGETS ${nmodl_python_target} DESTINATION ${NRN_INSTALL_PYTHON_PREFIX}/nmodl)
endforeach()

# =============================================================================
# Propagate targets to parent scope
# =============================================================================

set(NMODL_PYTHON_TARGETS
    ${NMODL_PYTHON_TARGETS}
    PARENT_SCOPE)

set(NMODL_DEFAULT_PYTHON_TARGET
    ${nmodl_default_python_target}
    PARENT_SCOPE)
