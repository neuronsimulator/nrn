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
  # Wheels: nmodl dlopens libpython then this .so. Do not DT_NEEDED libpython.
  add_library(pywrapper SHARED ${CMAKE_CURRENT_SOURCE_DIR}/wrapper.cpp)
  set_target_properties(pywrapper PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
else()
  # In-tree: BUILD_SHARED_LIBS would otherwise make a .so that testcodegen DT_NEEDs without
  # libpython (--as-needed), then fails at Catch discover with PyExc_ValueError.
  add_library(pywrapper STATIC ${CMAKE_CURRENT_SOURCE_DIR}/wrapper.cpp)
  set_property(TARGET pywrapper PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_compile_definitions(pyembed PRIVATE NMODL_STATIC_PYWRAPPER=1)
endif()

if(NRN_ENABLE_COVERAGE)
  target_compile_definitions(pywrapper PRIVATE NRN_ENABLE_COVERAGE)
  target_link_libraries(pywrapper PRIVATE util)
endif()

target_link_libraries(pywrapper PRIVATE fmt::fmt)

# CLI libpywrapper uses nanobind STABLE_ABI + limited-API Py_Initialize (no pybind11 embed).
set(_nmodl_python_inc ${NRN_DEFAULT_PYTHON_INCLUDES})
if(NOT _nmodl_python_inc)
  find_package(
    Python 3.12
    COMPONENTS Interpreter Development.Module
    REQUIRED)
  set(_nmodl_python_inc ${Python_INCLUDE_DIRS})
endif()
if(NOT TARGET nanobind_nmodl)
  make_nanobind_target(nanobind_nmodl "${_nmodl_python_inc}" STABLE_ABI)
endif()
target_include_directories(pyembed PRIVATE ${_nmodl_python_inc})
target_include_directories(pywrapper PRIVATE ${_nmodl_python_inc} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(pywrapper PRIVATE nanobind_nmodl)
target_link_libraries(pyembed PRIVATE ${CMAKE_DL_LIBS})
if(NOT NRN_LINK_AGAINST_PYTHON)
  if(APPLE)
    target_link_options(pywrapper PRIVATE "-Wl,-undefined,dynamic_lookup")
  elseif(UNIX)
    target_link_options(pywrapper PRIVATE "-Wl,--unresolved-symbols=ignore-all")
  endif()
else()
  target_compile_definitions(pyembed PRIVATE NMODL_STATIC_PYWRAPPER=1)
  set(_nmodl_embed_python ${NRN_DEFAULT_PYTHON_LIBRARIES})
  if(WIN32)
    get_filename_component(_nmodl_pylib_dir "${NRN_DEFAULT_PYTHON_LIBRARIES}" DIRECTORY)
    get_filename_component(_nmodl_pyprefix "${_nmodl_pylib_dir}" DIRECTORY)
    foreach(_cand "${_nmodl_pylib_dir}/python3.lib" "${_nmodl_pylib_dir}/libs/python3.lib"
                  "${_nmodl_pyprefix}/libs/python3.lib")
      if(EXISTS "${_cand}")
        set(_nmodl_embed_python "${_cand}")
        break()
      endif()
    endforeach()
  endif()
  target_link_libraries(pyembed PRIVATE ${_nmodl_embed_python} pywrapper)
endif()

# avoid _nmodl target if python bindings are disabled
if(NMODL_ENABLE_PYTHON_BINDINGS)
  if(NOT TARGET nanobind_nmodl)
    make_nanobind_target(nanobind_nmodl "${NRN_DEFAULT_PYTHON_INCLUDES}" STABLE_ABI)
  endif()
  add_library(
    _nmodl MODULE
    ${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/ast/ast_common.hpp
    ${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/pybind/pybind_utils.hpp
    ${NMODL_PROJECT_PLATLIB_SOURCE_DIR}/pybind/pynmodl.cpp ${PYBIND_GENERATED_SOURCES})
  add_dependencies(_nmodl lexer pyastgen util)
  target_link_libraries(_nmodl PRIVATE printer symtab visitor nanobind_nmodl)
  target_include_directories(_nmodl PRIVATE ${_nmodl_python_inc} ${CMAKE_CURRENT_BINARY_DIR})
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
    get_filename_component(_nmodl_pyprefix "${_nmodl_pylib_dir}" DIRECTORY)
    set(_nmodl_pylib "${PYTHON_LIBRARIES}")
    foreach(_cand "${_nmodl_pylib_dir}/python3.lib" "${_nmodl_pylib_dir}/libs/python3.lib"
                  "${_nmodl_pyprefix}/libs/python3.lib")
      if(EXISTS "${_cand}")
        set(_nmodl_pylib "${_cand}")
        break()
      endif()
    endforeach()
    target_link_libraries(_nmodl PRIVATE ${_nmodl_pylib})
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
