# - Try to find ReportingLib 
# Once done this will define
#  REPORTINGLIB_FOUND - System has ReportingLib 
#  REPORTINGLIB_INCLUDE_DIRS - The ReportingLib include directories
#  REPORTINGLIB_LIB_DIRS - The directory containing the ReportingLib libraries
#  REPORTINGLIB_LIBRARIES - The libraries needed to use ReportingLib

find_package(PkgConfig)

# find reportinglib path and includes
find_path(REPORTINGLIB_INCLUDE_DIR Records.h Report.h 
          HINTS ${REPORTINGLIB_ROOT} ${REPORTINGLIB_ROOT}/include ${REPORTINGLIB_INCLUDE_DIR} /usr/include
          PATH_SUFFIXES ReportingLib)

# Find the reportinglib libraries
find_library(REPORTINGLIB_LIBRARY NAMES ReportingLib
             HINTS ${REPORTINGLIB_ROOT} ${REPORTINGLIB_LIB_DIRS} /usr/lib ${REPORTINGLIB_ROOT}/lib )

set(REPORTINGLIB_LIB_DIRS ${REPORTINGLIB_LIBRARY})
set(REPORTINGLIB_INCLUDE_DIRS ${REPORTINGLIB_INCLUDE_DIR})
set(REPORTINGLIB_LIBRARIES ${REPORTINGLIB_LIBRARY})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set REPORTINGLIB_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ReportingLib DEFAULT_MSG
                                  REPORTINGLIB_LIBRARY REPORTINGLIB_INCLUDE_DIR )

mark_as_advanced(REPORTINGLIB_LIBRARY REPORTINGLIB_INCLUDE_DIR)
