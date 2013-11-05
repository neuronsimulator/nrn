# - Try to find Neuron 
# Once done this will define
#  NEURON_FOUND - System has Neuron 
#  NEURON_INCLUDE_DIRS - The Neuron include directories
#  NEURON_LIB_DIRS - The libraries needed to use Neuron
#  NEURON_BIN - Binaries needed to use Neuron 
#  NEURON_BIN_NRNIVMODL - Nrnivmodl binary provided by Neuron

find_package(PkgConfig)
#pkg_check_modules(PC_NEURON Neuron-7.2)
#set(LIBNEURON_DEFINITIONS ${PC_LIBXML_CFLAGS_OTHER})

# find neuron path and includes
find_path(NEURON_INCLUDE_DIR nrn/neuron.h nrn/nrnredef.h
          HINTS ${NEURON_ROOT} ${NEURON_ROOT}/include ${NEURON_INCLUDE_DIR} /usr/include
          PATH_SUFFIXES nrn )

# Find the neuron libraries
find_library(NEURON_LIBRARY NAMES nrniv nrnoc oc  
             HINTS ${NEURON_ROOT} ${NEURON_LIB_DIRS} /usr/lib ${NEURON_ROOT}/${CMAKE_SYSTEM_PROCESSOR}/lib )

# Find the neuron executables
find_program(NEURON_BIN_DIR NAMES nrniv nrnivmodl nrnoc
             HINTS ${NEURON_ROOT} ${NEURON_BIN} /usr/bin ${NEURON_ROOT}/${CMAKE_SYSTEM_PROCESSOR}/bin )
 
set(NEURON_LIB_DIRS ${NEURON_LIBRARY} )
set(NEURON_INCLUDE_DIRS ${NEURON_INCLUDE_DIR} )
set(NEURON_BIN ${NEURON_BIN_DIR} )
set(NEURON_BIN_NRNIVMODL ${NEURON_BIN_DIR}/nrnivmodl )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set NEURON_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Neuron DEFAULT_MSG
                                  NEURON_LIBRARY NEURON_INCLUDE_DIR NEURON_BIN_DIR)

mark_as_advanced(LIBNEURON_INCLUDE_DIR LIBNEURON_LIBRARY NEURON_BIN_DIR)
