# =============================================================================
# Copyright (C) 2016-2020 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# ~~~
# Macro sets up build rule for .cpp files from .mod files. Parameters:
# KEY name               : An arbitrary name to keep track of output .c files
# MODFILE input          : The path to the mod file
# OUTTYPE <SERIAL|ISPC>  : The output type (optional, defaults to serial)
#
# Because nmodl/mod2c_core wants to write the .cpp file in the same directory as the mod file,
# we copy the mod file to the binary directory first
#
# The macro appends the names of the output files to NMODL_${name}_OUTPUTS and the names of the
# mod files (without directories) to NMODL_${name}_MODS
# ~~~

macro(nmodl_to_cpp_target)
  # first parse the arguments
  set(options)
  set(oneValueArgs TARGET MODFILE KEY)
  set(multiValueArgs)
  cmake_parse_arguments(mod2c "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if("${mod2c_MODFILE}" STREQUAL "")
    message(FATAL_ERROR "mod2c_target requires a MODFILE argument")
  endif()
  if("${mod2c_KEY}" STREQUAL "")
    message(FATAL_ERROR "mod2c_target requires a KEY argument")
  endif()

  # get name of mod file and it's path
  get_filename_component(mod2c_source_ ${mod2c_MODFILE} ABSOLUTE)
  get_filename_component(mod2c_modname_ ${mod2c_MODFILE} NAME)

  # check for nmodl target
  if("${mod2c_TARGET}" STREQUAL "serial")
    string(REGEX REPLACE "\\.mod$" ".cpp" mod2c_outname_ "${mod2c_modname_}")
    set(nmodl_modearg "--c")
  elseif("${mod2c_TARGET}" STREQUAL "ispc")
    file(STRINGS ${mod2c_MODFILE} mod2c_mod_artcell REGEX "ARTIFICIAL_CELL")
    if(NOT "${mod2c_mod_artcell}" STREQUAL "")
      string(REGEX REPLACE "\\.mod$" ".cpp" mod2c_outname_ "${mod2c_modname_}")
      set(nmodl_modearg "--c")
    else()
      string(REGEX REPLACE "\\.mod$" ".ispc" mod2c_outname_ "${mod2c_modname_}")
      string(REGEX REPLACE "\\.mod$" ".cpp" mod2c_wrapper_outname_ "${mod2c_modname_}")
      set(nmodl_modearg "--ispc")
    endif()
  elseif("${mod2c_TARGET}" STREQUAL "")
    # default case
    string(REGEX REPLACE "\\.mod$" ".cpp" mod2c_outname_ "${mod2c_modname_}")
    set(nmodl_modearg "--c")
  else()
    message(
      SEND_ERROR
        "mod2c_target only supports serial and ispc generation for now: \"${mod2c_TARGET}\"")
  endif()

  set(mod2c_output_ "${CMAKE_CURRENT_BINARY_DIR}/${mod2c_outname_}")
  list(APPEND NMODL_${mod2c_KEY}_OUTPUTS "${mod2c_output_}")

  if(DEFINED mod2c_wrapper_outname_)
    set(mod2c_wrapper_output_ "${CMAKE_CURRENT_BINARY_DIR}/${mod2c_wrapper_outname_}")
    list(APPEND NMODL_${mod2c_KEY}_OUTPUTS "${mod2c_wrapper_output_}")
    unset(mod2c_wrapper_outname_)
  endif()

  list(APPEND NMODL_${mod2c_KEY}_MODS "${mod2c_modname_}")
  if(CORENRN_ENABLE_NMODL AND NMODL_FOUND)
    string(FIND "${NMODL_EXTRA_FLAGS_LIST}" "passes" pos)
    if(pos EQUAL -1)
      set(NMODL_FLAGS "passes;--inline;${NMODL_EXTRA_FLAGS_LIST}")
    else()
      string(REPLACE "passes" "passes;--inline" NMODL_FLAGS "${NMODL_EXTRA_FLAGS_LIST}")
    endif()
    add_custom_command(
      OUTPUT ${mod2c_output_} ${mod2c_wrapper_output_}
      DEPENDS ${mod2c_MODFILE} ${CORENRN_NMODL_BINARY}
      COMMAND ${CMAKE_COMMAND} -E copy "${mod2c_source_}" "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND
        ${CORENRN_NMODL_COMMAND} "${mod2c_modname_}" -o "${CMAKE_CURRENT_BINARY_DIR}" host
        ${nmodl_modearg} ${NMODL_FLAGS}
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  else()
    add_custom_command(
      OUTPUT ${mod2c_output_}
      DEPENDS ${mod2c_MODFILE} mod2c ${CORENRN_NMODL_BINARY}
      COMMAND ${CMAKE_COMMAND} -E copy "${mod2c_source_}" "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND ${CORENRN_NMODL_COMMAND} "${mod2c_modname_}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  endif()
endmacro()
