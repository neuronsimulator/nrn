cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)
project(NEURON C CXX)

# =============================================================================
# CMake common project settings
# =============================================================================
set(PROJECT_VERSION_MAJOR 7)
set(PROJECT_VERSION_MINOR 7)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

# =============================================================================
# Build options (boolean)
# =============================================================================
option(NRN_BUILD_SHARED "Build shared libraries (otherwise static library)" ON)
option(NRN_ENABLE_INTERVIEWS "Enable GUI with INTERVIEWS" ON)
option(NRN_ENABLE_LEGACY_FR "Use original faraday, R, etc. instead of 2017 nist constants" ON)
option(NRN_ENABLE_MECH_DLL_STYLE "Dynamically load nrnmech shared library" ON)
option(NRN_ENABLE_DISCRETE_EVENT_OBSERVER "Enable Observer to be a subclass of DiscreteEvent" ON)
option(NRN_ENABLE_PYTHON "Enable Python interpreter support (default python3 fallback to python)" ON)
option(NRN_ENABLE_THREADS "Allow use of Pthreads" ON)
option(NRN_ENABLE_MPI "Enable MPI support" ON)
option(NRN_ENABLE_MEMACS "Enable use of memacs" ON)
option(NRN_ENABLE_RX3D "Enable rx3d support" OFF)

# =============================================================================
# Build options (string)
# =============================================================================

# NEURON module installation:
#   - NO        : do not install
#   - YES       : install with --home in ${CMAKE_INSTALL_PREFIX}
#   - <string>  : install using other modes or locations using an appropriate
#                 string that goes after python setup.py install
# Dynamic Python version support:
#   - NO        : nrnpython interface is linked into libnrniv.so
#   - YES       : nrnpython interface consistent with default python3 (falling back to python)
#                 is built and loaded dynamically at run time (nrniv still works in the absence
#                 of any Python at all).
#   - <string>  : semicolon (;) separated list of python executable used to create a separate
#                 interface for each. When one of those versions of Python is lauched,
#                 "import neuron" will automatically load the appropriate module interface along
#                 with the rest of neuron. Alsom nrniv -pyexe <python> will work with any <python>
#                 in the list of python executables.
# Dynamic MPI support:
#   - NO        : nrnmpi is linked into libnrniv.so
#   - YES       : nrnmpi interface is consistent with default mpi is built and loaded dynamically
#                 at run time (nrniv still works in the absence of any mpi at all).
#   - <string>  : semicolon (;) separated list of MPI's bin directories to create a separate
#                 libnrnmpi_xxx.so interface for each. When nrniv is launched with the -mpi argument,
#                 the first mpi found will determine which interface is dynamically loaded."
set(NRN_ENABLE_MODULE_INSTALL YES CACHE STRING "Enable NEURON module installation (with setup.py)")
set(NRN_ENABLE_PYTHON_DYNAMIC "NO" CACHE STRING "Enable dynamic Python version support")
set(NRN_ENABLE_MPI_DYNAMIC "NO" CACHE STRING "Enable dynamic MPI library support")

# =============================================================================
# Include cmake modules
# =============================================================================
list(APPEND CMAKE_MODULE_PATH
            ${PROJECT_SOURCE_DIR}/cmake
            ${PROJECT_SOURCE_DIR}/cmake/modules)
include(CompilerHelper)
include(MacroHelper)
include(RpathHelper)
include(PlatformHelper)

# =============================================================================
# Find required packages
# =============================================================================
find_package(BISON)
find_package(FLEX)
find_package(readline)

# =============================================================================
# Enable MPI/Python/IV/Pthead if found
# =============================================================================
# enable mpi if found
if (NRN_ENABLE_MPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
  set(NRNMPI 1)
  set(PARANEURON 1)
else()
  set(NRNMPI 0)
  set(PARANEURON 0)
endif()

# enable interviews if found
if (NRN_ENABLE_INTERVIEWS)
  find_package(X11)
  if(NOT ${X11_FOUND})
    if (APPLE)
      message(FATAL_ERROR "You must install XQuartz from https://www.xquartz.org/ to build iv")
    else()
      message(FATAL_ERROR "You must install X11 to build iv e.g. 'apt install libx11-dev libxcomposite-dev' on Ubuntu")
    endif()
  endif()
  find_package(iv REQUIRED)
  include_directories(${X11_INCLUDE_DIR})
  set (HAVE_IV 1)
else()
  set(HAVE_IV 0)
endif()

# enable python support (interpreter needed to make hocusr.h from neuron.h, prefer Python 3)
set(Python_ADDITIONAL_VERSIONS 3.6)
find_package(PythonInterp REQUIRED)
if (NRN_ENABLE_PYTHON)
  find_package(PythonLibs REQUIRED)
  set(USE_PYTHON 1)
else()
  set(USE_PYTHON 0)
endif()

# enable threads if found
if (NRN_ENABLE_THREADS)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  set(USE_PTHREAD 1)
else()
  set(USE_PTHREAD 0)
endif()

# =============================================================================
# Add CMake modules AFTER setting options
# =============================================================================
include(ConfigFileSetting)
include(NeuronFileLists)
include(PythonDynamicHelper)
include(MPIDynamicHelper)

# =============================================================================
# Project version from git and project directories
# =============================================================================
add_subdirectory(cmake_config)
add_subdirectory(src/nrniv)
add_subdirectory(bin)
if (NRN_ENABLE_PYTHON)
  add_subdirectory(src/nrnpython)
endif()
if (NRN_MACOS_BUILD OR NOT ${READLINE_FOUND})
    add_subdirectory(src/readline)
    set(INTERNAL_READLINE readline)
    unset(Readline_LIBRARY CACHE)
endif()

# =============================================================================
# Install targets
# =============================================================================
# find headers to install
nrn_find_project_files(NRN_HEADERS_PATHS ${NRN_HEADERS_TO_INSTALL})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/lib DESTINATION share/nrn)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/demo DESTINATION share/nrn)
install(FILES
  ${PROJECT_BINARY_DIR}/share/lib/nrnunits.lib
  ${PROJECT_BINARY_DIR}/share/lib/nrn.defaults
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/nrn/lib
)
install(PROGRAMS
  ${CMAKE_CURRENT_SOURCE_DIR}/share/lib/cleanup
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/nrn/lib
)
install(FILES
  ${NRN_HEADERS_PATHS} ${PROJECT_BINARY_DIR}/src/oc/nrnpthread.h
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

# =============================================================================
# Print build status
# =============================================================================
message(STATUS "")
message(STATUS "Configured NEURON ${PROJECT_VERSION}")
message(STATUS "")
string(TOLOWER "${CMAKE_GENERATOR}" cmake_generator_tolower)
if(cmake_generator_tolower MATCHES "makefile")
  message(STATUS "Some things you can do now:")
  message(STATUS "--------------+--------------------------------------------------------------")
  message(STATUS "Command       |   Description")
  message(STATUS "--------------+--------------------------------------------------------------")
  message(STATUS "make install  | Will install NEURON to: ${CMAKE_INSTALL_PREFIX}")
  message(STATUS "              | Change the install location of NEURON using:")
  message(STATUS "              |     cmake <src_path> -DCMAKE_INSTALL_PREFIX=<install_path>")
  message(STATUS "make doc      | Build the API documentation, requires Sphinx")
  message(STATUS "make uninstall| Removes files installed by make install")
  message(STATUS "--------------+--------------------------------------------------------------")
  message(STATUS " Build option | Status")
  message(STATUS "--------------+--------------------------------------------------------------")
  message(STATUS "Interviews    | ${HAVE_IV}")
  message(STATUS "MPI           | ${NRNMPI}")
  if(NRN_ENABLE_MPI)
  message(STATUS "  INC         | ${MPI_INCLUDE_PATH}")
  message(STATUS "  LIB         | ${MPI_LIBRARY}")
  endif()
  message(STATUS "Python        | ${USE_PYTHON}")
  if(NRN_ENABLE_PYTHON)
  message(STATUS "  EXE         | ${PYTHON_EXECUTABLE}")
  message(STATUS "  LIB         | ${PYTHON_LIBRARY}")
  message(STATUS "  INC(2)      | ${NRNPYTHON_INCLUDE2}")
  message(STATUS "  INC(3)      | ${NRNPYTHON_INCLUDE3}")
  endif()
  message(STATUS "RXD           | ${NRN_ENABLE_RX3D}")
  message(STATUS "Shared        | ${NRN_BUILD_SHARED}")
  message(STATUS "--------------+--------------------------------------------------------------")
  message(STATUS " See documentation : https://www.neuron.yale.edu/neuron/")
  message(STATUS "--------------+--------------------------------------------------------------")
endif()
message(STATUS "")
