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
if(NRN_ENABLE_COVERAGE)
  set(NMODL_ODE_PY_PATH "${NMODL_PROJECT_PURELIB_SOURCE_DIR}/ode.py")
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

if(NRN_ENABLE_COVERAGE)
  target_compile_definitions(pywrapper PRIVATE NRN_ENABLE_COVERAGE)
  target_link_libraries(pywrapper PRIVATE util)
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
  # nanobind STABLE_ABI (CPython 3.12+ GIL). Separate from hoc's non-stable nanobind; the two ABIs
  # are isolated in one process.
  make_nanobind_target(nanobind_nmodl "${PYTHON_INCLUDE_DIRS}" STABLE_ABI)
  add_library(
    _nmodl MODULE
    ${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/ast/ast_common.hpp
    ${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/pybind/pybind_utils.hpp
    ${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/pybind/pynmodl.cpp ${PYBIND_GENERATED_SOURCES})
  add_dependencies(_nmodl lexer pyastgen util)
  target_link_libraries(_nmodl PRIVATE printer symtab visitor nanobind_nmodl)
  target_include_directories(_nmodl PRIVATE ${PYTHON_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
  # Windows import machinery has no .abi3.pyd suffix; Unix uses _nmodl.abi3.so.
  if(WIN32)
    set(_nmodl_suffix ".pyd")
  else()
    set(_nmodl_suffix ".abi3${CMAKE_SHARED_MODULE_SUFFIX}")
  endif()
  set_target_properties(
    _nmodl
    PROPERTIES PREFIX ""
               OUTPUT_NAME "_nmodl"
               SUFFIX "${_nmodl_suffix}"
               CXX_VISIBILITY_PRESET "hidden"
               VISIBILITY_INLINES_HIDDEN ON
               LIBRARY_OUTPUT_DIRECTORY ${NMODL_PROJECT_PURELIB_BINARY_DIR}
               LIBRARY_OUTPUT_DIRECTORY_DEBUG ${NMODL_PROJECT_PURELIB_BINARY_DIR}
               RUNTIME_OUTPUT_DIRECTORY ${NMODL_PROJECT_PURELIB_BINARY_DIR}
               RUNTIME_OUTPUT_DIRECTORY_DEBUG ${NMODL_PROJECT_PURELIB_BINARY_DIR})
  if(APPLE)
    target_link_options(_nmodl PRIVATE "-Wl,-undefined,dynamic_lookup")
  elseif(UNIX)
    target_link_options(_nmodl PRIVATE "-Wl,--unresolved-symbols=ignore-all")
  elseif(WIN32)
    # Limited API: python3.lib (python3.dll), not python3.XY.dll.
    get_filename_component(_nmodl_pylib_dir "${PYTHON_LIBRARIES}" DIRECTORY)
    if(EXISTS "${_nmodl_pylib_dir}/python3.lib")
      target_link_libraries(_nmodl PRIVATE "${_nmodl_pylib_dir}/python3.lib")
    else()
      target_link_libraries(_nmodl PRIVATE ${PYTHON_LIBRARIES})
    endif()
  endif()

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
    install(TARGETS _nmodl DESTINATION ${NRN_INSTALL_PYTHON_PREFIX}/nmodl)
  endif()
else()
  install(
    DIRECTORY ${CMAKE_BINARY_DIR}/lib/
    DESTINATION ${NRN_INSTALL_DATA_PREFIX}lib
    PATTERN "__pycache__" EXCLUDE)
endif()
