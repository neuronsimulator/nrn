# Often older version of flex is available in /usr. Even we set PATH for newer flex, CMake will set
# FLEX_INCLUDE_DIRS to /usr/include. This will result in compilation errors. Hence we check for flex
# include directory for the corresponding FLEX_EXECUTABLE. If found, we add that first and then we
# include include path from CMake.

get_filename_component(FLEX_BIN_DIR ${FLEX_EXECUTABLE} DIRECTORY)

# /usr/bin/flex uses /usr/include, which is already on the default include path. Adding that
# directory first picks up the wrong FlexLexer.h when a newer flex is also installed. Every other
# layout needs an explicit directory: a prefix keeps the header in ../include, and win_flex keeps it
# beside the real executable. Chocolatey's win_flex on PATH is a shim; the header is in
# lib/winflexbison*/tools.
if(NOT FLEX_BIN_DIR MATCHES "/usr/bin")
  set(_nrn_flex_inc_candidates)
  if(FLEX_INCLUDE_PATH)
    list(APPEND _nrn_flex_inc_candidates "${FLEX_INCLUDE_PATH}")
  endif()
  get_filename_component(_nrn_flex_prefix "${FLEX_BIN_DIR}" DIRECTORY)
  list(APPEND _nrn_flex_inc_candidates "${_nrn_flex_prefix}/include" "${FLEX_BIN_DIR}")
  if(FLEX_BIN_DIR MATCHES "[Cc]hocolatey")
    file(GLOB _nrn_choco_flex_headers "${_nrn_flex_prefix}/lib/winflexbison*/tools/FlexLexer.h")
    foreach(_nrn_choco_header IN LISTS _nrn_choco_flex_headers)
      get_filename_component(_nrn_choco_dir "${_nrn_choco_header}" DIRECTORY)
      list(APPEND _nrn_flex_inc_candidates "${_nrn_choco_dir}")
    endforeach()
  endif()
  foreach(_nrn_flex_inc IN LISTS _nrn_flex_inc_candidates)
    if(EXISTS "${_nrn_flex_inc}/FlexLexer.h")
      message(STATUS " Adding Flex include path as : ${_nrn_flex_inc}")
      include_directories("${_nrn_flex_inc}")
      break()
    endif()
  endforeach()
endif()
