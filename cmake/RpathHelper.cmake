# =============================================================================
# Set full RPATHs in build-tree, also set RPATHs in install for non-system libs
# =============================================================================

# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# when building, don't use the install RPATH already (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(LOADER_PATH_FLAG "@loader_path")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LOADER_PATH_FLAG "$ORIGIN")
else()
  set(LOADER_PATH_FLAG "")
endif()

# add the automatically determined parts of the RPATH which point to directories outside the build
# tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if(isSystemDir STREQUAL "-1")
  if(NRN_ENABLE_REL_RPATH)
    message(STATUS "Using relative RPATHs")
    set(CMAKE_INSTALL_RPATH "${LOADER_PATH_FLAG}/../lib")
  else()
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
endif()
