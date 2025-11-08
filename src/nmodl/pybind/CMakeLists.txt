# =============================================================================
# pybind targets
# =============================================================================
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

# build nmodl Python module under lib/python/neuron/nmodl
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${NMODL_PROJECT_PURELIB_BINARY_DIR})

file(READ ${NMODL_PROJECT_PURELIB_SOURCE_DIR}/ode.py NMODL_ODE_PY)
set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS ${NMODL_PROJECT_PURELIB_SOURCE_DIR}/ode.py)
if(WIN32)
  # MSVC can't handle long string literals, even if the documentation claims so See
  # https://developercommunity.visualstudio.com/t/c-string-literal-max-length-much-shorter-than-docu/758957
  string(REGEX REPLACE "\n\n" "\n)jiowi\" R\"jiowi(\n" NMODL_ODE_PY "${NMODL_ODE_PY}")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ode_py.hpp.inc ${CMAKE_CURRENT_BINARY_DIR}/ode_py.hpp
               @ONLY)

add_library(pyembed STATIC pyembed.cpp)
set_property(TARGET pyembed PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(pyembed PRIVATE util)
target_link_libraries(pyembed PRIVATE fmt::fmt)

if(NOT NRN_LINK_AGAINST_PYTHON)
  add_library(pywrapper SHARED ${CMAKE_CURRENT_SOURCE_DIR}/wrapper.cpp)
  set_target_properties(pywrapper PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
else()
  add_library(pywrapper ${CMAKE_CURRENT_SOURCE_DIR}/wrapper.cpp)
  set_property(TARGET pywrapper PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_compile_definitions(pyembed PRIVATE NMODL_STATIC_PYWRAPPER=1)
endif()

target_link_libraries(pywrapper PRIVATE fmt::fmt)

target_include_directories(pyembed PRIVATE ${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})
target_include_directories(pywrapper PRIVATE ${pybind11_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
target_include_directories(pywrapper PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
# ~~~
# pybind11::embed adds PYTHON_LIBRARIES to target_link_libraries. To avoid link to
# libpython, we can use `module` interface library from pybind11.
# ~~~
target_link_libraries(pyembed PRIVATE ${CMAKE_DL_LIBS})
if(NOT NRN_LINK_AGAINST_PYTHON)
  target_link_libraries(pywrapper PRIVATE pybind11::module)
else()
  target_link_libraries(pyembed PRIVATE ${PYTHON_LIBRARIES} pywrapper)
  target_link_libraries(pywrapper PRIVATE pybind11::embed)
endif()

# avoid _nmodl target if python bindings are disabled
if(NMODL_ENABLE_PYTHON_BINDINGS)
  # ~~~
  # Note that LTO causes link time errors with GCC 8. To avoid this, we disable LTO
  # for pybind using NO_EXTRAS. See #266.
  # ~~~
  pybind11_add_module(
    _nmodl NO_EXTRAS ${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/ast/ast_common.hpp
    ${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/pybind/pybind_utils.hpp
    ${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/pybind/pynmodl.cpp ${PYBIND_GENERATED_SOURCES})
  add_dependencies(_nmodl lexer pyastgen util)
  target_link_libraries(_nmodl PRIVATE printer symtab visitor pyembed)
  set_target_properties(_nmodl PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG
                                          ${NMODL_PROJECT_PURELIB_BINARY_DIR})

  if(MSVC)
    target_compile_options(_nmodl PRIVATE /bigobj)
  endif()

  # in case of wheel, python module shouldn't link to wrapper library
  if(NRN_LINK_AGAINST_PYTHON)
    target_link_libraries(_nmodl PRIVATE pywrapper)
  endif()
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
file(COPY ${NMODL_PROJECT_PURELIB_SOURCE_DIR}/ext DESTINATION ${NMODL_PROJECT_PURELIB_BINARY_DIR})

# =============================================================================
# Install python binding components
# =============================================================================
if(NOT NRN_LINK_AGAINST_PYTHON)
  install(TARGETS pywrapper DESTINATION ${NRN_INSTALL_DATA_PREFIX}lib)
  if(NMODL_ENABLE_PYTHON_BINDINGS)
    install(TARGETS _nmodl DESTINATION ${NRN_INSTALL_PYTHON_PREFIX}nmodl)
  endif()
else()
  install(
    DIRECTORY ${CMAKE_BINARY_DIR}/lib/
    DESTINATION ${NRN_INSTALL_DATA_PREFIX}lib
    PATTERN "__pycache__" EXCLUDE)
endif()
