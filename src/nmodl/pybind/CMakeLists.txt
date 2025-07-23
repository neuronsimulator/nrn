# =============================================================================
# pybind targets (for embedded interpreter)
# =============================================================================

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
  add_library(pywrapper SHARED wrapper.cpp)
  set_target_properties(pywrapper PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
else()
  add_library(pywrapper wrapper.cpp)
  set_property(TARGET pywrapper PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_compile_definitions(pyembed PRIVATE NMODL_STATIC_PYWRAPPER=1)
endif()

target_link_libraries(pywrapper PRIVATE fmt::fmt)

target_include_directories(pyembed PRIVATE ${PYBIND11_INCLUDE_DIR} ${NRN_DEFAULT_PYTHON_INCLUDES})
target_include_directories(pywrapper PRIVATE ${PYBIND11_INCLUDE_DIR} ${NRN_DEFAULT_PYTHON_INCLUDES}
                                             ${CMAKE_CURRENT_BINARY_DIR})
# ~~~
# pybind11::embed adds PYTHON_LIBRARIES to target_link_libraries. To avoid link to
# libpython, we can use `module` interface library from pybind11.
# ~~~
target_link_libraries(pyembed PRIVATE ${CMAKE_DL_LIBS})
if(NOT NRN_LINK_AGAINST_PYTHON)
  target_link_libraries(pywrapper PRIVATE pybind11::module)
else()
  target_link_libraries(pyembed PRIVATE ${NRN_DEFAULT_PYTHON_LIBRARIES} pywrapper)
  target_link_libraries(pywrapper PRIVATE pybind11::embed)
endif()

# =============================================================================
# Install python binding components
# =============================================================================
install(TARGETS pywrapper pyembed DESTINATION ${NRN_INSTALL_DATA_PREFIX}/lib)
