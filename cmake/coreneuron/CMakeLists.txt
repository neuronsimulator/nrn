# Ecole Polytechnique Federale de Lausanne
# Brain Mind Institute,
# Blue Brain Project
# (c) 2006-2013. All rights reserved.
#
# Author: Aleksandr Ovcharenko
# Core Bluron

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(cbluron)

set(RELEASE_VERSION OFF)  # OFF or ABI version
set(LAST_RELEASE 0.9.0)
set (VERSION_MAJOR "1")
set (VERSION_MINOR "0")
set (VERSION_PATCH "0")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

include(Common)
include(FindPackages)

if(MPI_FOUND)
  set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
  set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
endif(MPI_FOUND)

set(HEADERS
nrnconf.h
nrniv/arraypool.h
nrniv/htlist.h
nrniv/ivlist.h
nrniv/ivocvect.h
nrniv/ivtable.h
nrniv/netcon.h
nrniv/netcvode.h
nrniv/nrnhash.h
nrniv/nrnhash_alt.h
nrniv/nrniv_decl.h
nrniv/nrniv_mf.h
nrniv/nrnmutdec.h
nrniv/pool.h
nrniv/sptbinq.h
nrniv/sptree.h
nrniv/structpool.h
nrniv/tqueue.h
nrnoc/cabvars.h
nrnoc/md1redef.h
nrnoc/md2redef.h
nrnoc/membdef.h
nrnoc/membfunc.h
nrnoc/mpispike.h
nrnoc/multicore.h
nrnoc/nrnmpi.h
nrnoc/nrnmpidec.h
nrnoc/nrnmpiuse.h
nrnoc/nrnoc_decl.h
nrnoc/nrnoc_ml.h
nrnoc/nrnpthread.h
nrnmpi/nrnmpi_impl.h
)

set(SOURCES
nrnmpi/nrnmpi.c
nrnmpi/mpispike.c
nrnoc/capac.c
nrnoc/eion.c
nrnoc/finitialize.c
nrnoc/fadvance_core.c
nrnoc/solve_core.c
nrnoc/treeset_core.c
nrnoc/nrnoc_aux.c
nrnoc/register_mech.c
nrnoc/multicore.c
nrnoc/nrntimeout.c
nrnoc/stim.c
nrnoc/hh.c
nrniv/netpar.cpp
nrniv/netcvode.cpp
nrniv/cxprop.cpp
nrniv/cvodestb.cpp
nrniv/tqueue.cpp
nrniv/htlist.cpp
nrniv/ivlistimpl.cpp
nrniv/ivvect.cpp
nrniv/mk_mech.cpp
nrniv/nrn_setup.cpp
nrniv/output_spikes.cpp
)

set(LIBRARY cbluron)

include_directories(${CMAKE_SOURCE_DIR}/nrnmpi ${CMAKE_SOURCE_DIR}/nrnoc ${CMAKE_SOURCE_DIR}/nrniv)
add_library(${LIBRARY} ${HEADERS} ${SOURCES})
add_executable(special main.cpp)
target_link_libraries (special cbluron)

####### Including files for Packaging ##############
set_target_properties(${LIBRARY} PROPERTIES VERSION ${VERSION} SOVERSION ${VERSION_ABI} PUBLIC_HEADER "${HEADERS}")
install(TARGETS ${LIBRARY}
  PUBLIC_HEADER DESTINATION include
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/bin/special DESTINATION bin)

############ Packaging & Release ###################
#set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/bin" CACHE PATH "")

include(CPackConfig)
include(DoxygenRule)
include(PackageConfig)
include(BBPRelease)
