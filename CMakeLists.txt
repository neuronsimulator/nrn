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

set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

# =============================================================================
# CMake common project settings
# =============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# =============================================================================
# Include default build options
# =============================================================================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(BuildOptionDefaults)

# =============================================================================
# Build options (boolean)
# =============================================================================
option(NRN_ENABLE_SHARED "Build shared libraries (otherwise static library)"
       ${NRN_ENABLE_SHARED_DEFAULT})
option(NRN_ENABLE_INTERVIEWS "Enable GUI with INTERVIEWS" ${NRN_ENABLE_INTERVIEWS_DEFAULT})
option(NRN_ENABLE_LEGACY_FR "Use original faraday, R, etc. instead of 2019 nist constants"
       ${NRN_ENABLE_LEGACY_FR_DEFAULT})
option(NRN_ENABLE_MECH_DLL_STYLE "Dynamically load nrnmech shared library"
       ${NRN_ENABLE_MECH_DLL_STYLE_DEFAULT})
option(NRN_ENABLE_DISCRETE_EVENT_OBSERVER "Enable Observer to be a subclass of DiscreteEvent"
       ${NRN_ENABLE_DISCRETE_EVENT_OBSERVER_DEFAULT})
option(NRN_ENABLE_PYTHON "Enable Python interpreter support (default python3 fallback to python)"
       ${NRN_ENABLE_PYTHON_DEFAULT})
option(NRN_ENABLE_THREADS "Allow use of Pthreads" ${NRN_ENABLE_THREADS_DEFAULT})
option(NRN_ENABLE_MPI "Enable MPI support" ${NRN_ENABLE_MPI_DEFAULT})
option(NRN_ENABLE_MEMACS "Enable use of memacs" ${NRN_ENABLE_EMACS})
option(NRN_ENABLE_RX3D "Enable rx3d support" ${NRN_ENABLE_RX3D_DEFAULT})
option(NRN_ENABLE_CORENEURON "Enable CoreNEURON support" ${NRN_ENABLE_CORENEURON_DEFAULT})
option(NRN_ENABLE_TESTS "Enable unit tests" ${NRN_ENABLE_TESTS_DEFAULT})
# note that if CoreNEURON is enabled then it is not necessary to enable this option
option(NRN_ENABLE_MOD_COMPATIBILITY "Enable CoreNEURON compatibility for MOD files" ${NRN_ENABLE_MOD_COMPATIBILITY_DEFAULT})
option(NRN_USE_REL_RPATH "Use relative RPATH in binaries. for relocatable installs/Python" FALSE)

# =============================================================================
# Build options (string)
# =============================================================================
# ~~~
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
# ~~~
option(NRN_ENABLE_MODULE_INSTALL "Enable installation of NEURON Python module"
       ${NRN_ENABLE_MODULE_INSTALL_DEFAULT})
set(NRN_MODULE_INSTALL_OPTIONS
    "${NRN_MODULE_INSTALL_OPTIONS_DEFAULT}"
    CACHE STRING "setup.py options, everything after setup.py install")

option(NRN_ENABLE_PYTHON_DYNAMIC "Enable dynamic Python version support"
       ${NRN_ENABLE_PYTHON_DYNAMIC_DEFAULT})
set(NRN_PYTHON_DYNAMIC
    ""
    CACHE
      STRING
      "semicolon (;) separated list of python executables to create interface for (default python3)"
)

option(NRN_ENABLE_MPI_DYNAMIC "Enable dynamic MPI library support" OFF)
set(NRN_MPI_DYNAMIC
    ""
    CACHE
      STRING
      "semicolon (;) separated list of MPI bin directories to build against (default to first found mpi)"
)

set(NRN_RX3D_OPT_LEVEL
    "${NRN_RX3D_OPT_LEVEL_DEFAULT}"
    CACHE STRING "Optimization level for Cython generated files (non-zero may compile slowly)")

# =============================================================================
# Include cmake modules
# =============================================================================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
include(CompilerHelper)
include(MacroHelper)
include(RpathHelper)
include(PlatformHelper)
include(ExternalProjectHelper)
include(FindPythonModule)

# set CMAKE_BUILD_TYPE and associated flags using allowableBuildTypes and CMAKE_BUILD_TYPE_DEFAULT
set(allowableBuildTypes Custom Debug Release RelWithDebInfo Fast)
include(ReleaseDebugAutoFlags)

# =============================================================================
# Find required packages
# =============================================================================
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(Threads QUIET)
find_package(readline)
if(NRN_ENABLE_RX3D)
  if(NOT NRN_ENABLE_PYTHON)
    message(SEND_ERROR "NRN_ENABLE_RX3D requires NRN_ENABLE_PYTHON feature.")
  else()
    find_package(Cython REQUIRED)
  endif()
endif()
if(MINGW)
  find_package(Termcap REQUIRED)
endif()

# =============================================================================
# Setup Doxygen documentation
# =============================================================================
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  # generate Doxyfile with correct source paths
  configure_file(${PROJECT_SOURCE_DIR}/docs/Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile)
  add_custom_target(
    doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
endif()

# =============================================================================
# Enable MPI
# =============================================================================
if(NRN_ENABLE_MPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
  set(NRNMPI 1)
  set(PARANEURON 1)
else()
  set(NRNMPI 0)
  set(PARANEURON 0)
endif()

# =============================================================================
# Enable Interviews
# =============================================================================
if(NRN_ENABLE_INTERVIEWS)
  # x11 is not required on windows
  if(NOT NRN_WINDOWS_BUILD)
    find_package(X11 QUIET)
    if(NOT X11_FOUND)
      if(APPLE)
        message(SEND_ERROR "You must install XQuartz from https://www.xquartz.org/ to build iv")
      else()
        message(
          SEND_ERROR
            "You must install X11 to build iv e.g. 'apt install libx11-dev libxcomposite-dev' on Ubuntu"
        )
      endif()
    endif()
    include_directories(${X11_INCLUDE_DIR})
  endif()

  find_package(iv QUIET PATHS ${IV_DIR}/share/cmake ${IV_DIR})
  if(iv_FOUND)
    message(STATUS "Using external Interviews from ${IV_DIR}")
    get_target_property(IV_INCLUDE_DIR interviews INTERFACE_INCLUDE_DIRECTORIES)
  else()
    add_external_project(iv)
    include_directories(external/iv/src/include)
    set(IV_DIR ${PROJECT_SOURCE_DIR}/external/iv)
    set(IV_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/external/iv/src/include)
  endif()
  set(HAVE_IV 1)
else()
  set(HAVE_IV 0)
endif()

# =============================================================================
# Enable Python support
# =============================================================================
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

# =============================================================================
# Enable Threads support
# =============================================================================
if(NRN_ENABLE_THREADS)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  set(USE_PTHREAD 1)
else()
  set(USE_PTHREAD 0)
endif()

# =============================================================================
# Enable CoreNEURON support
# =============================================================================
if(NRN_ENABLE_CORENEURON)
  find_package(coreneuron QUIET PATHS ${CORENEURON_DIR}/share/cmake ${CORENEURON_DIR})
  if(coreneuron_FOUND)
    message(STATUS "Using external CoreNEURON from ${CORENEURON_DIR}")
  else()
    message(STATUS "Building CoreNEURON from submodule")
    add_external_project(coreneuron)
    set(CORENEURON_DIR ${PROJECT_SOURCE_DIR}/external/coreneuron)
  endif()
endif()

# =============================================================================
# Add helpder CMake modules AFTER setting options
# =============================================================================
include(NeuronFileLists)
include(MPIDynamicHelper)
include(ConfigFileSetting)
include(PythonDynamicHelper)

# =============================================================================
# Set install location for libraries (before src/nrniv)
# =============================================================================
# ~~~
# Classically, the install destination of the share folder for mac/linux has
# been <prefix>/share/nrn but for linux it has been <prefix>. For now we keep
# this distinction.
# Also, the classic location for shared libraries has been <prefix>/<arch>/lib
# and for max/linux we have move this to <inst>/lib. But windows has classically
# expected these shared libraries in <prefix>/bin (reduces the PATH and expected
# by ctypes in the neuron module.) So for now we keep that distinction as
# well. Setting these here as setup.py.in needs it.
# ~~~
if(MINGW)
  set(NRN_INSTALL_SHARE_DIR ${CMAKE_INSTALL_PREFIX})
  set(NRN_BUILD_SHARE_DIR ${CMAKE_BINARY_DIR})
  set(NRN_INSTALL_SHARE_LIB_DIR ${CMAKE_INSTALL_PREFIX}/bin)
else()
  set(NRN_INSTALL_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/nrn)
  set(NRN_BUILD_SHARE_DIR ${CMAKE_BINARY_DIR}/share/nrn)
  set(NRN_INSTALL_SHARE_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

# =============================================================================
# Add project directories AFTER CMake modules
# =============================================================================
add_subdirectory(src/nrniv)
add_subdirectory(bin)

if(NRN_ENABLE_PYTHON)
  add_subdirectory(src/nrnpython)
endif()

if(NRN_MACOS_BUILD OR NOT READLINE_FOUND)
  add_subdirectory(src/readline)
  set(INTERNAL_READLINE readline)
  unset(Readline_LIBRARY CACHE)
endif()

if(NRN_ENABLE_RX3D)
  add_subdirectory(share/lib/python/neuron/rxd/geometry3d)
endif()

if(NRN_MACOS_BUILD)
  add_subdirectory(src/mac)
endif()

if(MINGW)
  add_subdirectory(src/mswin)
endif()

# =============================================================================
# Add coding-conventions submodule if code formatting enabled
# =============================================================================
if(NEURON_CMAKE_FORMAT)
  if(NOT EXISTS external/coding-conventions/cpp)
    initialize_submodule(external/coding-conventions)
  endif()
  add_subdirectory(external/coding-conventions/cpp)
endif()

# =============================================================================
# ~~~
# Update hh.mod for CoreNEURON compatibility
# - Replace GLOBAL variable by RANHE
# - Comment out TABLE
# ~~~
# =============================================================================
if(NRN_ENABLE_CORENEURON OR NRN_ENABLE_MOD_COMPATIBILITY)
  set(GLOBAL_VAR_TOGGLE_COMMAND "'s/ GLOBAL minf/ RANGE minf/'")
  set(TABLE_VAR_TOGGLE_COMMAND "'s/ TABLE minf/ :TABLE minf/'")
else()
  set(GLOBAL_VAR_TOGGLE_COMMAND "'s/ RANGE minf/ GLOBAL minf/'")
  set(TABLE_VAR_TOGGLE_COMMAND "'s/ :TABLE minf/ TABLE minf/'")
endif()
separate_arguments(GLOBAL_VAR_TOGGLE_COMMAND UNIX_COMMAND "${GLOBAL_VAR_TOGGLE_COMMAND}")
separate_arguments(TABLE_VAR_TOGGLE_COMMAND UNIX_COMMAND "${TABLE_VAR_TOGGLE_COMMAND}")
add_custom_target(
  hh_update
  COMMAND sed ${GLOBAL_VAR_TOGGLE_COMMAND} ${CMAKE_SOURCE_DIR}/src/nrnoc/hh.mod >
          ${CMAKE_BINARY_DIR}/hh.mod.1
  COMMAND sed ${TABLE_VAR_TOGGLE_COMMAND} ${CMAKE_BINARY_DIR}/hh.mod.1 >
          ${CMAKE_BINARY_DIR}/hh.mod.2
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/hh.mod.2
          ${CMAKE_SOURCE_DIR}/src/nrnoc/hh.mod
  COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/hh.mod.1 ${CMAKE_BINARY_DIR}/hh.mod.2
  COMMENT "Update hh.mod for CoreNEURON compatibility"
  VERBATIM)
add_dependencies(nrniv_lib hh_update)

# =============================================================================
# Add tests if enabled
# =============================================================================
if(NRN_ENABLE_TESTS)
  add_external_project(catch2)
  set(CATCH_DIR ${PROJECT_SOURCE_DIR}/external/catch2)
  list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/external/catch2/contrib)
  include(CTest)
  include(Catch)
  find_python_module(pytest)
  if(NRN_ENABLE_PYTHON AND NOT PYTEST_FOUND)
    message(SEND_ERROR "pytest Python package is required.")
  endif()
  add_subdirectory(test)
endif()

# =============================================================================
# Install targets
# =============================================================================
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/lib
        DESTINATION ${NRN_INSTALL_SHARE_DIR}
        PATTERN "python" EXCLUDE)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/demo DESTINATION ${NRN_INSTALL_SHARE_DIR})
install(FILES ${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib
              ${PROJECT_BINARY_DIR}/share/nrn/lib/nrn.defaults
        DESTINATION ${NRN_INSTALL_SHARE_DIR}/lib)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/share/lib/cleanup
        DESTINATION ${NRN_INSTALL_SHARE_DIR}/lib)

# find headers to install
nrn_find_project_files(NRN_HEADERS_PATHS ${HEADER_FILES_TO_INSTALL})
file(COPY ${NRN_HEADERS_PATHS} ${PROJECT_BINARY_DIR}/src/oc/nrnpthread.h
     DESTINATION ${PROJECT_BINARY_DIR}/include)
install(DIRECTORY ${PROJECT_BINARY_DIR}/include DESTINATION ${CMAKE_INSTALL_PREFIX})

# for external coreneuron, nrnivmodl-core should be install
if(coreneuron_FOUND)
  install(PROGRAMS ${CORENEURON_DIR}/bin/nrnivmodl-core DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
  install(FILES ${CORENEURON_DIR}/bin/nrnivmodl_core_makefile
          DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif()

# =============================================================================
# Copy bash executable for windows
# =============================================================================
if(MINGW)
  # ~~~
  # nrniv.cpp calls nrnpyenv.sh with absolute path to bash.exe
  # this is sufficient on the build machine since a full
  # development environment exists. On the users install machine
  # using a setup.exe distribution, the setup.ex will contain a
  # minimal development environment with sufficient mingw programs
  # to allow nrnpyenv.sh to work. (see nrn/mingw_files/nrnmingwenv.sh)
  # ~~~
  find_file(BASH_EXE bash.exe DOC "DOS path to bash.exe")
  message(STATUS "Found bash.exe at ${BASH_EXE}")
  if("${BASH_EXE}" STREQUAL "BASH_EXE-NOTFOUND")
    set(BASH_EXE "f:/msys64/usr/bin/bash.exe")
    message(WARNING "Can not find bash.exe, trying to use ${BASH_EXE}")
  endif()
  install(PROGRAMS ${BASH_EXE} DESTINATION ${CMAKE_INSTALL_PREFIX}/mingw/usr/bin)
endif()

# =============================================================================
# Print build status
# =============================================================================

# just for printing the compiler flags in the build status
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
if(BUILD_TYPE_UPPER MATCHES "CUSTOM")
  set(COMPILER_FLAGS "${CMAKE_CXX_FLAGS}")
else()
  set(COMPILER_FLAGS "${CMAKE_CXX_FLAGS_${BUILD_TYPE_UPPER}}")
endif()

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
  message(STATUS "make docs     | Build the API documentation")
  message(STATUS "make uninstall| Removes files installed by make install (todo)")
  message(STATUS "--------------+--------------------------------------------------------------")
  message(STATUS " Build option | Status")
  message(STATUS "--------------+--------------------------------------------------------------")
  message(STATUS "BUILD_TYPE    | ${CMAKE_BUILD_TYPE} (allowed: ${allowableBuildTypes})")
  message(STATUS "COMPILE FLAGS | ${COMPILER_FLAGS}")
  message(STATUS "Shared        | ${NRN_ENABLE_SHARED}")
  message(STATUS "Legacy FR     | ${NRN_ENABLE_LEGACY_FR}")
  message(STATUS "MPI           | ${NRN_ENABLE_MPI}")
  if(NRN_ENABLE_MPI)
    message(STATUS "  INC         | ${MPI_INCLUDE_PATH}")
    message(STATUS "  LIB         | ${MPI_LIBRARY}")
    message(STATUS "  DYNAMIC     | ${NRN_ENABLE_MPI_DYNAMIC}")
  endif()
  message(STATUS "Python        | ${NRN_ENABLE_PYTHON}")
  if(NRN_ENABLE_PYTHON)
    message(STATUS "  EXE         | ${NRN_DEFAULT_PYTHON_EXECUTABLE}")
    message(STATUS "  INC         | ${NRN_DEFAULT_PYTHON_INCLUDE_DIRS}")
    message(STATUS "  LIB         | ${NRN_DEFAULT_PYTHON_LIBRARIES}")
    message(STATUS "  MODULE      | ${NRN_ENABLE_MODULE_INSTALL}")
    message(STATUS "  DYNAMIC     | ${NRN_ENABLE_PYTHON_DYNAMIC}")
    if(NRN_ENABLE_PYTHON_DYNAMIC)
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
        message(STATUS "    INSTALL C | ${exe} setup.py install ${NRN_MODULE_INSTALL_OPTIONS}")
      endforeach(val)
    endif()
  endif()
  message(STATUS "RX3D          | ${NRN_ENABLE_RX3D}")
  if(${NRN_ENABLE_RX3D})
    message(STATUS "  OptLevel    | ${NRN_RX3D_OPT_LEVEL}")
  endif()
  message(STATUS "Interviews    | ${NRN_ENABLE_INTERVIEWS}")
  if(NRN_ENABLE_INTERVIEWS)
    message(STATUS "  PATH        | ${IV_DIR}")
    message(STATUS "  INC         | ${IV_INCLUDE_DIR}")
    message(STATUS "  X11 (INC)   | ${X11_INCLUDE_DIR}")
    if(IV_ENABLE_X11_DYNAMIC)
      message(STATUS "              | IV_ENABLE_X11_DYNAMIC ${IV_ENABLE_X11_DYNAMIC}")
      message(STATUS "              | IV_ENABLE_SHARED ${IV_ENABLE_SHARED}")
    endif()
  endif()
  message(STATUS "CoreNEURON    | ${NRN_ENABLE_CORENEURON}")
  if(NRN_ENABLE_CORENEURON)
    message(STATUS "  PATH        | ${CORENEURON_DIR}")
  endif()
  message(STATUS "--------------+--------------------------------------------------------------")
  message(STATUS " See documentation : https://www.neuron.yale.edu/neuron/")
  message(STATUS "--------------+--------------------------------------------------------------")
endif()
message(STATUS "")
