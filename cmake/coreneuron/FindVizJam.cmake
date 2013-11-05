include(FindPackageHandleStandardArgs)

# Find scripts basically stolen from VizJam
set(_vizjam_required)
if(VizJam_FIND_REQUIRED)
  set(_vizjam_required REQUIRED)
endif()
if(VizJam_FIND_QUIETLY)
  set(_vizjam_quiet QUIET)
endif()

# find and parse eq/client/version.h [new location]
find_path(_vizjam_INCLUDE_DIR vizjam/version.h
  HINTS ${CMAKE_SOURCE_DIR}/../../.. $ENV{VIZJAM_ROOT} ${VIZJAM_ROOT}
  PATH_SUFFIXES include
  PATHS /usr /usr/local /opt/local /opt
)

if(NOT _vizjam_INCLUDE_DIR OR 
   NOT EXISTS "${_vizjam_INCLUDE_DIR}/vizjam/version.h")
  find_path(_vizjam_INCLUDE_DIR vizjam/version.h
    HINTS ${CMAKE_SOURCE_DIR}/../../.. $ENV{VIZJAM_ROOT} ${VIZJAM_ROOT}
    PATH_SUFFIXES include
    PATHS /usr /usr/local /opt/local /opt
  )
  if(_vizjam_INCLUDE_DIR AND EXISTS "${_vizjam_INCLUDE_DIR}/vizjam/version.h")
    set(_vizjam_Version_file "${_vizjam_INCLUDE_DIR}/vizjam/version.h")
  endif()
endif()

if(_vizjam_Version_file)
# Try to ascertain the version...
  #if("${_vizjam_INCLUDE_DIR}" MATCHES "\\.framework$" AND
  #    NOT EXISTS "${_vizjam_Version_file}")
  #  set(_vizjam_Version_file "${_vizjam_INCLUDE_DIR}/Headers/version.h")
  #endif()

  file(READ "${_vizjam_Version_file}" _vizjam_Version_contents)
  string(REGEX REPLACE ".*define VIZJAM_VERSION_MAJOR[ \t]+([0-9]+).*"
    "\\1" VIZJAM_VERSION_MAJOR ${_vizjam_Version_contents})
  string(REGEX REPLACE ".*define VIZJAM_VERSION_MINOR[ \t]+([0-9]+).*"
    "\\1" VIZJAM_VERSION_MINOR ${_vizjam_Version_contents})
  string(REGEX REPLACE ".*define VIZJAM_VERSION_PATCH[ \t]+([0-9]+).*"
    "\\1" VIZJAM_VERSION_PATCH ${_vizjam_Version_contents})
  string(REGEX REPLACE ".*define VIZJAM_SOVERSION[ \t]+([0-9]+).*"
    "\\1" VIZJAM_SOVERSION ${_vizjam_Version_contents})
  if(NOT VIZJAM_SOVERSION GREATER 1)
    set(VIZJAM_SOVERSION)
  endif()

  set(VIZJAM_VERSION "${VIZJAM_VERSION_MAJOR}.${VIZJAM_VERSION_MINOR}.${VIZJAM_VERSION_PATCH}"
    CACHE INTERNAL "The version of VizJam which was detected")
else()
  set(_vizjam_EPIC_FAIL TRUE)
  if(_vizjam_output)
    message(${_vizjam_version_output_type}
      "Can't find VizJam header file version.h.")
  endif()
endif()

# Version checking
if(VizJam_FIND_VERSION AND VIZJAM_VERSION)
  if(VizJam_FIND_VERSION_EXACT)
    if(NOT VIZJAM_VERSION VERSION_EQUAL ${VizJam_FIND_VERSION})
      set(_eq_version_not_exact TRUE)
    endif()
  else()
    # version is too low
    if(NOT VIZJAM_VERSION VERSION_EQUAL ${VizJam_FIND_VERSION} AND 
        NOT VIZJAM_VERSION VERSION_GREATER ${VizJam_FIND_VERSION})
      set(_eq_version_not_high_enough TRUE)
    endif()
  endif()
endif()

# Finding the libraries
find_library(_vizjam_LIBRARY vizjam PATH_SUFFIXES lib
  HINTS ${CMAKE_SOURCE_DIR}/../../.. $ENV{VIZJAM_ROOT} ${VIZJAM_ROOT}
  PATHS /usr /usr/local /opt/local /opt
)
find_library(_vizjam_boost_LIBRARY vizjam_boost${CMAKE_SHARED_LIBRARY_SUFFIX}
  PATH_SUFFIXES lib
  HINTS ${CMAKE_SOURCE_DIR}/../../.. $ENV{VIZJAM_ROOT} ${VIZJAM_ROOT}
  PATHS /usr /usr/local /opt/local /opt
)
find_library(_vizjam_sip_LIBRARY vizjam_sip${CMAKE_SHARED_LIBRARY_SUFFIX}
  PATH_SUFFIXES lib
  HINTS ${CMAKE_SOURCE_DIR}/../../.. $ENV{VIZJAM_ROOT} ${VIZJAM_ROOT}
  PATHS /usr /usr/local /opt/local /opt
)

# Inform the users with an error message based on what version they
# have vs. what version was required.
if(_vizjam_version_not_high_enough)
  set(_vizjam_EPIC_FAIL TRUE)
  message(${_vizjam_version_output_type}
    "Version ${VizJam_FIND_VERSION} or higher of VizJam is required. "
    "Version ${VIZJAM_VERSION} was found in ${_vizjam_Version_file}.")
elseif(_vizjam_version_not_exact)
  set(_vizjam_EPIC_FAIL TRUE)
  message(${_vizjam_version_output_type}
    "Version ${VizJam_FIND_VERSION} of VizJam is required exactly. "
    "Version ${VIZJAM_VERSION} was found.")
else()
  if(VizJam_FIND_REQUIRED)
    if(_vizjam_LIBRARY MATCHES "_vizjam_LIBRARY-NOTFOUND")
      message(FATAL_ERROR "Missing the VizJam library.\n"
        "Consider using CMAKE_PREFIX_PATH or the VIZJAM_ROOT environment variable. "
        "See the ${CMAKE_CURRENT_LIST_FILE} for more details.")
    endif()
  endif()
  find_package_handle_standard_args(VizJam DEFAULT_MSG
                                    _vizjam_LIBRARY _vizjam_INCLUDE_DIR)
endif()

if(_vizjam_EPIC_FAIL)
  # Zero out everything, we didn't meet version requirements
  set(VIZJAM_FOUND FALSE)
  set(_vizjam_LIBRARY)
  set(_vizjam_boost_LIBRARY)
  set(_vizjam_sip_LIBRARY)
  set(_vizjam_INCLUDE_DIR)
else()
# set(VIZJAM_DEB_DEPENDENCIES "vizjam (>=${VIZJAM_VERSION})")
endif()

set(VIZJAM_INCLUDE_DIRS ${_vizjam_INCLUDE_DIR} ${GLEW_MX_INCLUDE_DIRS})
set(VIZJAM_LIBRARIES 
  ${_vizjam_LIBRARY} 
  ${_vizjam_boost_LIBRARY}
  ${_vizjam_sip_LIBRARY}
)
get_filename_component(VIZJAMLIBRARY_DIR ${_vizjam_LIBRARY} PATH)

if(VIZJAM_FOUND)
  message("-- Found VIZJAM TODO_VERSION/TODO_VERSION_ABI in ${VIZJAM_INCLUDE_DIRS}"
    ";${VIZJAM_LIBRARIES}")
endif()
