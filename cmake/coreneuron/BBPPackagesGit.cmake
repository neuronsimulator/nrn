# used by BBPPackages.cmake, don't use directly

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/oss)
find_package(Git)

if(NOT GIT_EXECUTABLE)
  return()
endif()
include(UpdateFile)

file(GLOB_RECURSE RPMS RELATIVE ${CMAKE_SOURCE_DIR}/redhat *.rpm)
set(ENTRIES ${RPMS})

file(GLOB_RECURSE DEBS RELATIVE ${CMAKE_SOURCE_DIR}/ubuntu *.deb)
list(SORT DEBS)
list(REVERSE DEBS)
set(DEB_BASENAME_LAST)
foreach(DEB ${DEBS})
  if(NOT DEB MATCHES "^apt.*")
    string(REGEX MATCH "\\/.+_[0-9]+\\.[0-9]+\\.[0-9]+" DEB_BASENAME ${DEB})
    if(DEB_BASENAME STREQUAL DEB_BASENAME_LAST)
      file(REMOVE "${CMAKE_SOURCE_DIR}/ubuntu/${DEB}")
    else()
      list(APPEND ENTRIES ${CMAKE_SOURCE_DIR}/ubuntu/${DEB})
      set(DEB_BASENAME_LAST ${DEB_BASENAME})
    endif()
  endif()
endforeach()

execute_process(COMMAND "${GIT_EXECUTABLE}" add ${ENTRIES}
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/redhat")
