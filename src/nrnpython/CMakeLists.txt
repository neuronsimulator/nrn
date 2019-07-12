
set(NRN_SRCDIR ${PROJECT_SOURCE_DIR})

set(NRNPYTHON_DEFINES "")

set(NRNPYTHON_EXEC ${PYTHON_EXECUTABLE})

set(NRNPYTHON_PYVER ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})

#reset since no " allowed
set(PACKAGE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
set(NRN_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)

set(IV_LIBDIR "${iv_LIB}")

set(CC ${CMAKE_C_COMPILER})
set(CXX ${CMAKE_CXX_COMPILER})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MAC_DARWIN_TRUE "")
  set(MAC_DARWIN_FALSE "\#")
else()
  set(MAC_DARWIN_TRUE "\#")
  set(MAC_DARWIN_FALSE "")
endif()

if (ENABLE_RX3D)
  set(BUILD_RX3D "1")
else()
  set(BUILD_RX3D "0")
endif()

set(READLINE_SOSUFFIX "")

set(setup_extra_link_args "extra_link_args") # or #

set(npy_pyver10 "")

if (ENABLE_INTERVIEWS)
 set(IVHINES "interviews")
else()
 set(IVHINES "")
endif()

if (ENABLE_MEMACS)
  set(BUILD_MEMACS_TRUE "")
else()
  set(BUILD_MEMACS_TRUE "#")
endif()

set(BUILD_NRNMPI_DYNAMIC_FALSE "#")

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/inithoc.cpp
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)
configure_ac_file(setup.py src/nrnpython)

if ("${INSTALL_NEURON_MODULE}" EQUAL "NO")
  message(STATUS "neuron module will not be automatically installed")
else()
  install(CODE "
    message(STATUS \"neuron module install using ${INSTALL_NEURON_MODULE}\")
    execute_process(COMMAND ${PYTHON_EXECUTABLE} setup.py install ${INSTALL_NEURON_MODULE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  ")
endif()
