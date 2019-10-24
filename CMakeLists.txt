cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)
project(NEURON C CXX)

# =============================================================================
# CMake common project settings
# =============================================================================
set(PROJECT_VERSION_MAJOR 7)
set(PROJECT_VERSION_MINOR 8)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Empty or one of Debug, Release, RelWithDebInfo")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

# =============================================================================
# Build options (boolean)
# =============================================================================
option(NRN_ENABLE_SHARED "Build shared libraries (otherwise static library)" ON)
option(NRN_ENABLE_INTERVIEWS "Enable GUI with INTERVIEWS" ON)
option(NRN_ENABLE_LEGACY_FR "Use original faraday, R, etc. instead of 2017 nist constants" ON)
option(NRN_ENABLE_MECH_DLL_STYLE "Dynamically load nrnmech shared library" ON)
option(NRN_ENABLE_DISCRETE_EVENT_OBSERVER "Enable Observer to be a subclass of DiscreteEvent" ON)
option(NRN_ENABLE_PYTHON "Enable Python interpreter support (default python3 fallback to python)" ON)
option(NRN_ENABLE_THREADS "Allow use of Pthreads" ON)
option(NRN_ENABLE_MPI "Enable MPI support" ON)
option(NRN_ENABLE_MEMACS "Enable use of memacs" ON)
option(NRN_ENABLE_RX3D "Enable rx3d support" ON)
option(NRN_ENABLE_CORENEURON "Enable CoreNEURON support" ON)

# =============================================================================
# Build options (string)
# =============================================================================

# NEURON module installation:
#   - OFF       : do not install
#   - ON        : install with --home in ${CMAKE_INSTALL_PREFIX}
#   - <string>  : install using other modes or locations using an appropriate
#                 string that goes after python setup.py install
# Dynamic Python version support:
#   - OFF       : nrnpython interface is linked into libnrniv.so
#   - ON        : nrnpython interface consistent with default python3 (falling back to python)
#                 is built and loaded dynamically at run time (nrniv still works in the absence
#                 of any Python at all).
#   - <string>  : semicolon (;) separated list of python executable used to create a separate
#                 interface for each. When one of those versions of Python is launched,
#                 "import neuron" will automatically load the appropriate module interface along
#                 with the rest of neuron. Also nrniv -pyexe <python> will work with any <python>
#                 in the list of python executables.
# Dynamic MPI support:
#   - OFF       : nrnmpi is linked into libnrniv.so
#   - ON        : nrnmpi interface consistent with default mpi is built and loaded dynamically
#                 at run time (nrniv still works in the absence of any mpi at all).
#   - <string>  : semicolon (;) separated list of MPI's bin directories to create a separate
#                 libnrnmpi_xxx.so interface for each. When nrniv is launched with the -mpi argument,
#                 the first mpi found will determine which interface is dynamically loaded."
# Rx3D Cython generated files compiler optimization level. 0 is default.

option(NRN_ENABLE_MODULE_INSTALL "Enable installation of NEURON Python module" ON)
set(NRN_MODULE_INSTALL_OPTIONS "" CACHE STRING "setup.py options, leave empty for --home <install prefix>")

option(NRN_ENABLE_PYTHON_DYNAMIC "Enable dynamic Python version support" OFF)
set(NRN_PYTHON_DYNAMIC "" CACHE STRING "semicolon (;) separated list of python executables to create interface for (default python3)")

option(NRN_ENABLE_MPI_DYNAMIC "Enable dynamic MPI library support" OFF)
set(NRN_MPI_DYNAMIC "" CACHE STRING "semicolon (;) separated list of MPI bin directories to build against (default to first found mpi)")

set(NRN_RX3D_OPT_LEVEL "0" CACHE STRING "Integer optimization level for cython generated files. Nonzero may compile very slowly.")

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
include(ExternalProjectHelper)
include(FindCython)

# =============================================================================
# Find required packages
# =============================================================================
find_package(BISON)
find_package(FLEX)
find_package(Threads)
find_package(readline)
if (NRN_ENABLE_RX3D)
  find_package(Cython)
endif()

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
if (NRN_ENABLE_INTERVIEWS AND NOT NRN_WINDOWS_BUILD)
  find_package(X11)
  if(NOT X11_FOUND)
    if(APPLE)
      message(FATAL_ERROR "You must install XQuartz from https://www.xquartz.org/ to build iv")
    else()
      message(FATAL_ERROR "You must install X11 to build iv e.g. 'apt install libx11-dev libxcomposite-dev' on Ubuntu")
    endif()
  endif()
  include_directories(${X11_INCLUDE_DIR})
endif()
if (NRN_ENABLE_INTERVIEWS)
  find_package(iv QUIET PATHS ${IV_DIR}/share/cmake ${IV_DIR})
  if(${iv_FOUND})
    message(STATUS "Using external Interviews from ${IV_DIR}")
    get_target_property(IV_INCLUDE_DIR interviews INTERFACE_INCLUDE_DIRECTORIES)
  else()
    message(STATUS "Building Interviews from submodule")
    add_external_project(iv)
    include_directories(external/iv/src/include)
    set(IV_DIR ${PROJECT_SOURCE_DIR}/external/iv)
    set(IV_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/external/iv/src/include)
  endif()
  set(HAVE_IV 1)
else()
  set(HAVE_IV 0)
endif()

# enable python support (interpreter needed to make hocusr.h from neuron.h, prefer Python 3)
find_package(PythonInterp REQUIRED)
if(NRN_ENABLE_PYTHON)
  find_package(PythonLibsNew ${PYTHON_VERSION_MAJOR} REQUIRED)
  set(NRN_DEFAULT_PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE})
  set(NRN_DEFAULT_PYTHON_LIBRARIES ${PYTHON_LIBRARIES})
  set(NRN_DEFAULT_PYTHON_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS})
  set(USE_PYTHON 1)
else()
  set(USE_PYTHON 0)
endif()

# enable threads if found
if(NRN_ENABLE_THREADS)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  set(USE_PTHREAD 1)
else()
  set(USE_PTHREAD 0)
endif()

# TODO check if this is optional with ncurses
if (MINGW)
  find_package(Termcap REQUIRED)
endif()

# enable CoreNEURON support
if (NRN_ENABLE_CORENEURON)
  find_package(coreneuron QUIET PATHS ${CORENEURON_DIR}/share/cmake ${CORENEURON_DIR})
  if(${coreneuron_FOUND})
    message(STATUS "Using external CoreNEURON from ${CORENEURON_DIR}")
  else()
    message(STATUS "Building CoreNEURON from submodule")
    add_external_project(coreneuron)
    set(CORENEURON_DIR ${PROJECT_SOURCE_DIR}/external/coreneuron)
  endif()
endif()

# Classically, the install destination of the share folder
# for mac/linux has been
# <prefix>/share/nrn
# but for linux it has been <prefix>. For now we keep this distinction.
# Also, the classic location for shared libraries has been
# <prefix>/<arch>/lib and for max/linux we have move this to
# <inst>/lib
# But windows has classically expected these shared libraries in
# <prefix>/bin (reduces the PATH and expected by ctypes in the neuron module.)
# So for now we keep that distinction as well.
# Setting these here as setup.py.in needs it
set(nrn_dest_share_dir ${CMAKE_INSTALL_PREFIX}/share/nrn)
set(nrn_dest_sharedlibs ${CMAKE_INSTALL_PREFIX}/lib)
if (MINGW)
  set(nrn_dest_share_dir ${CMAKE_INSTALL_PREFIX})
  set(nrn_dest_sharedlibs ${CMAKE_INSTALL_PREFIX}/bin)
endif()

# =============================================================================
# Add CMake modules AFTER setting options
# =============================================================================
include(NeuronFileLists)
include(MPIDynamicHelper)
include(ConfigFileSetting)
include(PythonDynamicHelper)

# =============================================================================
# Project version from git and project directories
# =============================================================================
add_subdirectory(src/nrniv)
add_subdirectory(bin)

if (NRN_ENABLE_PYTHON)
  add_subdirectory(src/nrnpython)
endif()
if(NRN_MACOS_BUILD OR NOT READLINE_FOUND)
  add_subdirectory(src/readline)
  set(INTERNAL_READLINE readline)
  unset(Readline_LIBRARY CACHE)
endif()
if (NRN_ENABLE_RX3D)
  add_subdirectory(share/lib/python/neuron/rxd/geometry3d)
endif()

# =============================================================================
# Install targets
# =============================================================================
# find headers to install
nrn_find_project_files(NRN_HEADERS_PATHS ${HEADER_FILES_TO_INSTALL})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/lib DESTINATION ${nrn_dest_share_dir})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/demo DESTINATION ${nrn_dest_share_dir})
install(FILES
  ${PROJECT_BINARY_DIR}/share/lib/nrnunits.lib
  ${PROJECT_BINARY_DIR}/share/lib/nrn.defaults
  DESTINATION ${nrn_dest_share_dir}/lib
)
install(PROGRAMS
  ${CMAKE_CURRENT_SOURCE_DIR}/share/lib/cleanup
  DESTINATION ${nrn_dest_share_dir}/lib
)
install(FILES
  ${NRN_HEADERS_PATHS} ${PROJECT_BINARY_DIR}/src/oc/nrnpthread.h
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

#for external coreneuron, nrnivmodl-core should be install
if (NRN_ENABLE_CORENEURON AND ${coreneuron_FOUND})
  install(PROGRAMS
    ${CORENEURON_DIR}/bin/nrnivmodl-core
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
  install(FILES
    ${CORENEURON_DIR}/bin/nrnivmodl_core_makefile
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif()

if (MINGW)
  # nrniv.cpp calls nrnpyenv.sh with absolute path to bash.exe
  # this is sufficient on the build machine since a full
  # development environment exists. On the users install machine
  # using a setup.exe distribution, the setup.ex will contain a
  # minimal development environment with sufficient mingw programs
  # to allow nrnpyenv.sh to work. (see nrn/mingw_files/nrnmingwenv.sh)
  find_file(nrnbash bash.exe DOC "DOS path to bash.exe")
  message(STATUS "Found bash.exe at ${nrnbash}")
  if ("${nrnbash}" STREQUAL "nrnbash-NOTFOUND")
    set(nrnbash "f:/msys64/usr/bin/bash.exe")
  endif()
  install(PROGRAMS
    ${nrnbash}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/mingw/usr/bin
  )
endif()

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
  message(STATUS "Interviews    | ${NRN_ENABLE_INTERVIEWS}")
  if(NRN_ENABLE_INTERVIEWS)
    message(STATUS "  PATH        | ${IV_DIR}")
    message(STATUS "  INC         | ${IV_INCLUDE_DIR}")
  endif()
  message(STATUS "MPI           | ${NRN_ENABLE_MPI}")
  if(NRN_ENABLE_MPI)
    message(STATUS "  INC         | ${MPI_INCLUDE_PATH}")
    message(STATUS "  LIB         | ${MPI_LIBRARY}")
  endif()
  message(STATUS "Python        | ${NRN_ENABLE_PYTHON}")
  if(NRN_ENABLE_PYTHON)
    message(STATUS "  EXE         | ${NRN_DEFAULT_PYTHON_EXECUTABLE}")
    message(STATUS "  INC         | ${NRN_DEFAULT_PYTHON_INCLUDE_DIRS}")
    message(STATUS "  LIB         | ${NRN_DEFAULT_PYTHON_LIBRARIES}")
    message(STATUS "  DYNAMIC     | ${NRN_ENABLE_PYTHON_DYNAMIC}")
    if (NRN_ENABLE_PYTHON_DYNAMIC)
      list(LENGTH NRN_PYTHON_EXE_LIST _num_pythons)
      math(EXPR num_pythons "${_num_pythons} - 1")
      foreach(val RANGE ${num_pythons})
        list(GET NRN_PYTHON_EXE_LIST ${val} exe)
        list(GET NRN_PYTHON_VER_LIST ${val} version)
        list(GET NRN_PYTHON_INCLUDE_LIST ${val} include)
        list(GET NRN_PYTHON_LIB_LIST ${val} lib)
        message(STATUS "    EXE       | ${exe}")
        message(STATUS "    INC       | ${include}")
        message(STATUS "    LIB       | ${lib}")
        message(STATUS "    MODULE    | ${exe} setup.py install ${NRN_MODULE_INSTALL_OPTIONS}")
      endforeach(val)
    endif()
  endif()
  message(STATUS "RX3D          | ${NRN_ENABLE_RX3D}")
  if (${NRN_ENABLE_RX3D})
    message(STATUS "  OptLevel    | ${NRN_RX3D_OPT_LEVEL}")
  endif()
  message(STATUS "Shared        | ${NRN_ENABLE_SHARED}")
  message(STATUS "CoreNEURON    | ${NRN_ENABLE_CORENEURON}")
  if(NRN_ENABLE_CORENEURON)
    message(STATUS "  PATH        | ${CORENEURON_DIR}")
  endif()
  message(STATUS "--------------+--------------------------------------------------------------")
  message(STATUS " See documentation : https://www.neuron.yale.edu/neuron/")
  message(STATUS "--------------+--------------------------------------------------------------")
endif()
message(STATUS "")
