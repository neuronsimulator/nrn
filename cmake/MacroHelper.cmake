# =============================================================================
# Helper functions used in the project
# =============================================================================

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)

# =============================================================================
# Check if directory related to DIR exist by compiling code
# =============================================================================
macro(nrn_check_dir_exists HEADER VARIABLE)
  # code template to check existance of DIR
  set(CONFTEST_DIR_TPL "
    #include <sys/types.h>
    #include <@dir_header@>

    int main () {
      if ((DIR *) 0)
        return 0\;
      return 0\;
    }")
  # first get header file
  check_include_files(${HEADER} HAVE_HEADER)
  if(${HAVE_HEADER})
    # if header is found, create a code from template
    string(REPLACE "@dir_header@"
                   ${HEADER}
                   CONFTEST_DIR
                   "${CONFTEST_DIR_TPL}")
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c ${CONFTEST_DIR})
    # try to compile
    try_compile(RESULT_VAR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c)
    if (${RESULT_VAR})
      set(RESULT_VAR 1)
    else()
      set(RESULT_VAR 0)
    endif()
    set(${VARIABLE} ${RESULT_VAR})
    message(STATUS "nrn_check_dir_existss ${HEADER}: ${RESULT_VAR}")
    file(REMOVE "conftest.c")
  endif()
endmacro()

# =============================================================================
# Check if given type exist by compiling code
# =============================================================================
macro(nrn_check_type_exists HEADER TYPE DEFAULT_TYPE VARIABLE)
  # code template to check existance of specific type
  set(CONFTEST_TYPE_TPL "
    #include <@header@>

    int main () {
      if (sizeof (@type@))
        return 0\;
      return 0\;
    }")
  string(REPLACE "@header@"
                 ${HEADER}
                 CONFTEST_TYPE
                 "${CONFTEST_TYPE_TPL}")
  string(REPLACE "@type@"
                 ${TYPE}
                 CONFTEST_TYPE
                 "${CONFTEST_TYPE}")
  file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c ${CONFTEST_TYPE})

  try_compile(RESULT_VAR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c)
  if(NOT ${RESULT_VAR})
    set(${VARIABLE} ${DEFAULT_TYPE})
  endif()
  message(STATUS "nrn_check_type_exists ${VARIABLE}: ${RESULT_VAR}")
  file(REMOVE "conftest.c")
endmacro()

# =============================================================================
# Check return type of signal
# =============================================================================
macro(nrn_check_signal_return_type VARIABLE)
  # code template to check signal support
  set(CONFTEST_RETSIGTYPE "
    #include <sys/types.h>
    #include <signal.h>

    int main () {
      return *(signal (0, 0)) (0) == 1;
    }")
  file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c ${CONFTEST_RETSIGTYPE})
  try_compile(RESULT_VAR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c)
  if(${RESULT_VAR})
    set(${VARIABLE} int)
  else()
    set(${VARIABLE} void)
  endif()
  file(REMOVE "conftest.c")
endmacro()

# =============================================================================
# Transform CMAKE_CURRENT_SOURCE_DIR/file.in to CMAKE_CURRENT_BINARY_DIR/file
# =============================================================================
# Just as autoconf transforms file.in into file, this macro transforms
# CMAKE_CURRENT_SOURCE_DIR/file.in into CMAKE_CURRENT_BINARY_DIR/file.
# This first copies with some replacement the file.in to cmake_file.in
# so that the normal cmake configure_file command works to make a proper
# cmake_file. Then that is compared to a possibly existing file and
# if different copies file_cmk to file. This prevent recompilation of
# .o files that depend on file. The dir arg is the path relative to
# PROJECT_SOURCE_DIR and PROJECT_BINARY_DIR.
macro(nrn_configure_file file dir)
  set(bin_dir ${PROJECT_BINARY_DIR}/${dir})
  file(MAKE_DIRECTORY ${bin_dir})
  execute_process(COMMAND sed "s/\#undef *\\(.*\\)/\#cmakedefine \\1 @\\1@/"
    INPUT_FILE ${PROJECT_SOURCE_DIR}/${dir}/${file}.in
    OUTPUT_FILE ${bin_dir}/cmake_${file}.in)
  configure_file(${bin_dir}/cmake_${file}.in ${bin_dir}/cmake_${file} @ONLY)
  execute_process(COMMAND cmp -s ${bin_dir}/cmake_${file} ${bin_dir}/${file}
    RESULT_VARIABLE result)
  if (${result} EQUAL 0)
    file(REMOVE ${bin_dir}/cmake_${file})
  else()
    file(RENAME ${bin_dir}/cmake_${file} ${bin_dir}/${file})
  endif()
  file(REMOVE ${bin_dir}/cmake_${file}.in)
endmacro()

# =============================================================================
# Perform check_include_files and add it to NRN_HEADERS_INCLUDE_LIST if exist
# =============================================================================
macro(nrn_check_include_files filename variable)
  check_include_files(${filename} ${variable})
  if (${variable})
    list(APPEND NRN_HEADERS_INCLUDE_LIST ${filename})
  endif()
endmacro()

# =============================================================================
# Perform check_symbol_exists using NRN_HEADERS_INCLUDE_LIST if empty header_list
# =============================================================================
macro(nrn_check_symbol_exists name header_list variable)
  string(COMPARE EQUAL "${header_list}" "" empty)
  if (${empty})
    check_symbol_exists("${name}" "${NRN_HEADERS_INCLUDE_LIST}" ${variable})
  else()
    check_symbol_exists("${name}" "${header_list}" ${variable})
  endif()
endmacro()

# =============================================================================
# Perform check_cxx_symbol_exists using NRN_HEADERS_INCLUDE_LIST if empty header_list
# =============================================================================
# note that sometimes, though it should have succeeded, cc  fails but c++ succeeds
macro(nrn_check_cxx_symbol_exists name header_list variable)
  string(COMPARE EQUAL "${header_list}" "" empty)
  if (${empty})
    check_cxx_symbol_exists("${name}" "${NRN_HEADERS_INCLUDE_LIST}" ${variable})
  else()
    check_cxx_symbol_exists("${name}" "${header_list}" ${variable})
  endif()
endmacro()

# =============================================================================
# Append arguments to given list in the form of prefix/argument
# =============================================================================
macro(nrn_create_file_list list_name prefix)
  foreach(name ${ARGN})
    list(APPEND ${list_name} ${prefix}/${name})
  endforeach(name)
endmacro()

# =============================================================================
# Copy file from source to destination in clobber mode (i.e. no overwrite)
# =============================================================================
macro(nrn_copy_file_without_overwrite source destination)
  execute_process(COMMAND cp -n ${source} ${destination})
endmacro()

# =============================================================================
# Set string with double quotes
# =============================================================================
macro(nrn_set_string variable value)
  set(${variable} \"${value}\")
endmacro()

# =============================================================================
# Given list of file names, find their path in project source tree
# =============================================================================
macro(nrn_find_project_files list_name)
  foreach(name ${ARGN})
    execute_process(
      COMMAND find ${PROJECT_SOURCE_DIR}/src -name ${name}
      RESULTS_VARIABLE result
      OUTPUT_VARIABLE filepath
    )
    if ( (NOT result EQUAL 0) OR filepath STREQUAL "")
      message(FATAL_ERROR " ${name} not found in ${PROJECT_SOURCE_DIR}/src")
    else()
      string(REGEX REPLACE "\n$" "" filepath "${filepath}")
      list(APPEND ${list_name} ${filepath})
    endif()
  endforeach(name)
endmacro()

# =============================================================================
# Utility macro to print all matching CMake variables
# =============================================================================
# example usage : nrn_print_matching_variables("[Mm][Pp][Ii]")
macro (nrn_print_matching_variables prefix_regex)
  get_cmake_property(variable_names VARIABLES)
  list (SORT variable_names)
  foreach (variable ${variable_names})
    if (variable MATCHES "^${prefix_regex}")
      message(NOTICE " ${variable} ${${variable}}")
    endif()
  endforeach()
endmacro()

# =============================================================================
# Run nocmodl to convert NMODL to C
# =============================================================================
macro(nocmodl_mod_to_c modfile_basename)
  add_custom_command(OUTPUT ${modfile_basename}.c
                     COMMAND nocmodl ${modfile_basename}.mod
                     COMMAND sed "'s/_reg()/_reg_()/'" ${modfile_basename}.c > ${modfile_basename}.c.tmp
                     COMMAND mv ${modfile_basename}.c.tmp ${modfile_basename}.c
                     DEPENDS ${PROJECT_BINARY_DIR}/bin/nocmodl ${modfile_basename}.mod
		     WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/src/nrniv)
endmacro()
