cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)
project(nrn C CXX)

# =============================================================================
# CMake common project settings
# =============================================================================
set(PROJECT_VERSION_MAJOR 7)
set(PROJECT_VERSION_MINOR 7)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# needed in several subdirectoy configurations
set (prefix ${CMAKE_INSTALL_PREFIX})
set (host_cpu ${CMAKE_SYSTEM_PROCESSOR})
set (exec_prefix ${prefix})
set (bindir \${exec_prefix}/bin)
set (libdir \${exec_prefix}/lib)
set (PACKAGE "\"nrn\"")

option(NRN_BUILD_SHARED "Build libraries shared or static" ON)
option(ENABLE_INTERVIEWS "Link with INTERVIEWS GUI library" ON)
option(ENABLE_LegacyFR "Original faraday, R, etc. instead of 2017 nist constants in share/lib/nrnunits.lib" ON)
option(ENABLE_NRNMECH_DLL_STYLE "Dynamically load nrnmech shared library" ON)
option(ENABLE_DISCRETE_EVENT_OBSERVER "Set to OFF if do not want Observer to be a subclass of DiscreteEvent" ON)
option(ENABLE_PYTHON "Python interpreter available -- python3 fallback to python" ON)
option(ENABLE_THREADS "Allow use of pthreads" ON)
option(ENABLE_MPI "Allow use of MPI" ON)
option(ENABLE_MEMACS "Allow use of memacs" ON)
option(ENABLE_RX3D "Allow use of rx3d" OFF)

set(INSTALL_NEURON_MODULE "YES" CACHE STRING "\"NO\", means do not \
install the neuron module.  \"Yes\", means to install with --home in \
CMAKE_INSTALL_PREFIX.  For installation using other modes or locations \
use an appropriate string that goes after python setup.py install")

set(NRNPYTHON_DYNAMIC "NO" CACHE STRING "\"NO\", means the nrnpython
interface is linked into libnrniv.so.  \"YES\" means the nrnpython
interface consistent with default python3 (falling back to python) is
built and loaded dynamically at run time (nrniv still works in the
absence of any Python at all).  A ';' separated list of executable
pythons are used to create a separate interface for each.  When one of
those versions of Python is lauched,
  import neuron
will automatically load the appropriate module interface along with the
rest of neuron.  (And
  nrniv -pyexe <python>
will work with any <python> in the list of python executables.")

set(NRNMPI_DYNAMIC "NO" CACHE STRING "\"No\", means the nrnmpi
is linked into libnrniv.so.  \"YES\", means the nrnmpi interface
consistent with default mpi is built and loaded dynamically at run
time (nrniv still works in the absence of any mpi at all). A ';'
separated list of paths to mpi package bin directories is used to
create a separate libnrnmpi_xxx.so interface for each. When nrniv
is launched with the -mpi argument, the first mpi found will determine
which interface is dynamically loaded.")

set(USING_CMAKE_FALSE "#")
set(USING_CMAKE_TRUE "")

# =============================================================================
# Compile static libraries with hidden visibility
# =============================================================================
#set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# =============================================================================
# Find required packages
# =============================================================================
message(STATUS "CHECKING FOR X11")
find_package(BISON)
find_package(FLEX)

if (ENABLE_MPI)
  find_package(MPI REQUIRED)
  # TODO : include this for only selective target
  include_directories(${MPI_INCLUDE_PATH})
  set(NRNMPI 1)
  set(PARANEURON 1)
else()
  set(NRNMPI 0)
  set(PARANEURON 0)
endif()

if (ENABLE_INTERVIEWS)
  find_package(X11)
  if(NOT ${X11_FOUND})
    if (APPLE)
      message(FATAL_ERROR "You must install XQuartz from https://www.xquartz.org/ to build iv")
    else()
      find_package(X11 REQUIRED)
    endif()
  endif()
  include_directories(${X11_INCLUDE_DIR})
  find_package(iv REQUIRED)
  set (HAVE_IV 1)
else()
  set (HAVE_IV 0)
endif()

# python or python3 is also needed to make hocusr.h from neuron.h
set(Python_ADDITIONAL_VERSIONS 3.6) # 3.x priority, then 2.x
find_package(PythonInterp REQUIRED) #currently active
message(NOTICE "ENABLE_PYTHON ${ENABLE_PYTHON}")
if (ENABLE_PYTHON)
  message(NOTICE " inside ENABLE_PYTHON")
  find_package(PythonLibs REQUIRED) # consistent with above
  set(USE_PYTHON 1)
  message(NOTICE "USE_PYTHON ${USE_PYTHON}")
else()
  set(USE_PYTHON 0)
endif()

if (ENABLE_THREADS)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  set(USE_PTHREAD 1)
else()
  set(USE_PTHREAD 0)
endif()

# =============================================================================
# Include cmake modules
# =============================================================================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(RpathHelper)
include(ConfigACFile)
include(NrnPyDynamic)
include(NrnMPIDynamic)

# =============================================================================
# Project version from git and project directories
# =============================================================================
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

add_definitions(-DHAVE_CONFIG_H)

if(${CMAKE_SYSTEM_NAME} MATCHES "CYGWIN")
  set(CYGWIN 1)
endif()

add_subdirectory(cmkconfig)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/lib
  DESTINATION share/nrn)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/demo
  DESTINATION share/nrn)
install(FILES ${PROJECT_BINARY_DIR}/share/lib/nrnunits.lib
  ${PROJECT_BINARY_DIR}/share/lib/nrn.defaults
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/nrn/lib
)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/share/lib/cleanup
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/nrn/lib
)

# paths to *.h files for installing into <prefix>/include
my_find_files(inst_inc
  cabvars.h    hoc_membf.h   neuron.h      nrnjava.h     options.h
  cspmatrix.h  hocparse.h    nmodlmutex.h  nrnoc_ml.h    scoplib_ansi.h
  cspredef.h   md1redef.h    nrn_ansi.h    scoplib.h
  hocassrt.h   md2redef.h    nrnapi.h      nrnran123.h   section.h
  hocdec.h     membdef.h     nrnassrt.h    nrnredef.h    spconfig.h
  hocgetsym.h  membfunc.h    nrncvode.h    oc_ansi.h     spmatrix.h
  hoc.h        multicore.h   nrnisaac.h    ocfunc.h
  hoclist.h    multisplit.h  nrniv_mf.h    ocmisc.h
)
install(FILES ${inst_inc} ${PROJECT_BINARY_DIR}/src/oc/nrnpthread.h
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

find_package(readline)

# if readline not found or if we are on OSX, use internal readline
# also clear library variable
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR NOT ${READLINE_FOUND})
    add_subdirectory(src/readline)
    set(INTERNAL_READLINE readline)
    unset(Readline_LIBRARY CACHE)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -undefined dynamic_lookup")
endif()

add_subdirectory(src/nrniv)
add_subdirectory(bin)
if (ENABLE_PYTHON)
  add_subdirectory(src/nrnpython)
endif()
