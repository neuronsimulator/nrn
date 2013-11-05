# - Try to find Bluron 
# Once done this will define
#  BLURON_FOUND - System has Bluron 
#  BLURON_INCLUDE_DIRS - The Bluron include directories
#  BLURON_LIB_DIRS - The libraries needed to use Bluron
#  BLURON_BIN - Binaries needed to use Bluron 
#  BLURON_BIN_NRNIVMODL - Nrnivmodl binary provided by Bluron

find_package(PkgConfig)

# find neuron path and includes
find_path(BLURON_INCLUDE_DIR nrn/neuron.h nrn/nrnredef.h
          HINTS ${BLURON_ROOT} ${BLURON_ROOT}/include ${BLURON_INCLUDE_DIR} /usr/include
          PATH_SUFFIXES nrn )

# Find the neuron libraries
find_library(BLURON_LIBRARY NAMES nrniv nrnoc oc  
             HINTS ${BLURON_ROOT} ${BLURON_LIB_DIRS} /usr/lib ${BLURON_ROOT}/${CMAKE_SYSTEM_PROCESSOR}/lib )

# Find the neuron executables
find_path(BLURON_BIN_DIR NAMES nrniv nrnivmodl nrnoc
          PATHS ${BLURON_ROOT}/${CMAKE_SYSTEM_PROCESSOR}/bin /usr
          HINTS ${BLURON_ROOT} $ENV{BLURON_ROOT}
          PATH_SUFFIXES bin )
 
set(BLURON_LIB_DIRS ${BLURON_LIBRARY} )
set(BLURON_INCLUDE_DIRS ${BLURON_INCLUDE_DIR} )
set(BLURON_BIN ${BLURON_BIN_DIR})
set(BLURON_BIN_NRNIVMODL ${BLURON_BIN_DIR}/nrnivmodl )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set BLURON_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Bluron DEFAULT_MSG
                                  BLURON_LIBRARY BLURON_INCLUDE_DIR BLURON_BIN_DIR)

mark_as_advanced(LIBBLURON_INCLUDE_DIR LIBBLURON_LIBRARY BLURON_BIN_DIR)
