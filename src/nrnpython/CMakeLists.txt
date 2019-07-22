
set(NRN_SRCDIR ${PROJECT_SOURCE_DIR})

set(NRNPYTHON_DEFINES "")

# if NRN_PYTHON_DYNAMIC then the following three will be
# determined from the actual pyexe that runs setup.py
set(NRNPYTHON_EXEC ${PYTHON_EXECUTABLE})
set(NRNPYTHON_PYVER ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
set(npy_pyver10 "")

#reset since no " allowed
set(PACKAGE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
set(NRN_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)

set(IV_LIBDIR "${iv_LIB}")

set(CC ${CMAKE_C_COMPILER})
set(CXX ${CMAKE_CXX_COMPILER})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(MAC_DARWIN_TRUE "")
  set(MAC_DARWIN_FALSE "\#")
  set(setup_extra_link_args "#extra_link_args")
else()
  set(MAC_DARWIN_TRUE "\#")
  set(MAC_DARWIN_FALSE "")
  set(setup_extra_link_args "extra_link_args")
endif()

if (NRN_ENABLE_RX3D)
  set(BUILD_RX3D "1")
else()
  set(BUILD_RX3D "0")
endif()

set(READLINE_SOSUFFIX "")

if (NRN_ENABLE_INTERVIEWS)
 set(IVHINES "interviews")
else()
 set(IVHINES "")
endif()

if (NRN_ENABLE_MEMACS)
  set(BUILD_MEMACS_TRUE "")
else()
  set(BUILD_MEMACS_TRUE "#")
endif()

set(BUILD_NRNMPI_DYNAMIC_FALSE "#")

set(BUILD_NRNPYTHON_DYNAMIC_TRUE "")
set(BUILD_NRNPYTHON_DYNAMIC_FALSE "#")
if ("${NRN_ENABLE_PYTHON_DYNAMIC}" MATCHES "NO")
  set(BUILD_NRNPYTHON_DYNAMIC_TRUE "#")
  set(BUILD_NRNPYTHON_DYNAMIC_FALSE "")
  set(pyexelist ${PYTHON_EXECUTABLE})
elseif ("${NRN_ENABLE_PYTHON_DYNAMIC}" MATCHES "YES")
  set(pyexelist ${PYTHON_EXECUTABLE})
else()
  set(pyexelist ${NRN_ENABLE_PYTHON_DYNAMIC})
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/inithoc.cpp
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)
nrn_configure_file(setup.py src/nrnpython)

message(NOTICE: " NRN_NRNPYTHON_SRC_FILES ${NRN_NRNPYTHON_SRC_FILES}")

if ("${NRN_ENABLE_MODULE_INSTALL}" MATCHES "NO")
  message(STATUS "neuron module will not be automatically installed")
else()
  set(pyinst "${NRN_ENABLE_MODULE_INSTALL}")
  if ("${pyinst}" MATCHES "YES")
    set(pyinst "--home ${CMAKE_INSTALL_PREFIX}")
  endif()
  foreach (pyexe ${pyexelist})
    message(STATUS " Configured to install neuron module with the command: ${pyexe} setup.py install ${pyinst}")
  endforeach(pyexe)

#can get the include dir from distutils.sysconfig.get_python_inc()
  if (NOT "${NRN_ENABLE_PYTHON_DYNAMIC}" MATCHES "NO")
    set(idir
      . ../oc ../nrnoc ../ivoc ../nrniv ../ivos ../gnu ../mesch
      ../nrnmpi
      ${PROJECT_BINARY_DIR}/src/nrnpython
      ${PROJECT_BINARY_DIR}/src/ivos
      ${PROJECT_BINARY_DIR}/src/oc
      ${iv_INCLUDE}
    )

    if (NRNPYTHON_INCLUDE2)
      add_library(nrnpython2 SHARED ${NRN_NRNPYTHON_SRC_FILES})
      target_include_directories(nrnpython2 BEFORE PUBLIC
        ${NRNPYTHON_INCLUDE2} ${idir}
      )
      add_dependencies(nrnpython2 nrniv_lib)
      install(TARGETS nrnpython2 DESTINATION lib)
    endif()

    if (NRNPYTHON_INCLUDE3)
      add_library(nrnpython3 SHARED ${NRN_NRNPYTHON_SRC_FILES})
      add_dependencies(nrnpython3 nrniv_lib)
      target_include_directories(nrnpython3 BEFORE PUBLIC
        ${NRNPYTHON_INCLUDE3} ${idir}
      )
      install(TARGETS nrnpython3 DESTINATION lib)
    endif()
  endif()

  foreach(pyexe ${pyexelist})
    install(CODE "
      message(STATUS \" neuron module install with command: ${pyexe} setup.py install ${pyinst}\")
      execute_process(COMMAND ${pyexe} setup.py install ${pyinst}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )
    ")
  endforeach(pyexe)
endif()
