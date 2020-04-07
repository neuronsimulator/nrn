# =============================================================================
# Helper functions used in the project
# =============================================================================

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)

set(CMAKE_REQUIRED_QUIET TRUE)

# =============================================================================
# Check if directory related to DIR exists by compiling code
# =============================================================================
macro(nrn_check_dir_exists HEADER VARIABLE)
  # code template to check existence of DIR
  set(CONFTEST_DIR_TPL
      "
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
    string(REPLACE "@dir_header@" ${HEADER} CONFTEST_DIR "${CONFTEST_DIR_TPL}")
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c ${CONFTEST_DIR})
    # try to compile
    try_compile(RESULT_VAR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c)
    if(${RESULT_VAR})
      set(${VARIABLE} 1)
    else()
      set(${VARIABLE} 0)
    endif()
    file(REMOVE "conftest.c")
  endif()
endmacro()

# =============================================================================
# Check if given type exists by compiling code
# =============================================================================
macro(nrn_check_type_exists HEADER TYPE DEFAULT_TYPE VARIABLE)
  # code template to check existence of specific type
  set(CONFTEST_TYPE_TPL
      "
    #include <@header@>

    int main () {
      if (sizeof (@type@))
        return 0\;
      return 0\;
    }")
  string(REPLACE "@header@" ${HEADER} CONFTEST_TYPE "${CONFTEST_TYPE_TPL}")
  string(REPLACE "@type@" ${TYPE} CONFTEST_TYPE "${CONFTEST_TYPE}")
  file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c ${CONFTEST_TYPE})

  try_compile(RESULT_VAR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c)
  if(NOT ${RESULT_VAR})
    set(${VARIABLE} ${DEFAULT_TYPE})
  endif()
  file(REMOVE "conftest.c")
endmacro()

# =============================================================================
# Check return type of signal
# =============================================================================
macro(nrn_check_signal_return_type VARIABLE)
  # code template to check signal support
  set(CONFTEST_RETSIGTYPE
      "
    #include <sys/types.h>
    #include <signal.h>

    int main () {
      return *(signal (0, 0)) (0) == 1;
    }")
  file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c ${CONFTEST_RETSIGTYPE})
  try_compile(RESULT_VAR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/conftest.c)
  if(RESULT_VAR)
    set(${VARIABLE} int)
  else()
    set(${VARIABLE} void)
  endif()
  file(REMOVE "conftest.c")
endmacro()

# =============================================================================
# Transform PROJECT_SOURCE_DIR/sdir/sfile.in to PROJECT_BINARY_DIR/bdir/bfile
# =============================================================================
# ~~~
# Just as autoconf transforms file.in into file, this 4 arg macro transforms
# PROJECT_SOURCE_DIR/sdir/sfile.in into PROJECT_BINARY_DIR/bdir/bfile . The
# shorter two arg form transforms PROJECT_SOURCE_DIR/dir/file.in into
# PROJECT_BINARY_DIR/dir/file
# This first copies with some replacement the sfile.in to _cmake_tmp_bfile.in
# so that the normal cmake configure_file command works to make a proper
# cmake_file. Then that is compared to a possibly existing bfile and,
# if different, copies _cmake_tmp_bfile to bfile. This prevents recompilation of
# .o files that depend on unchanged bfile. The sdir arg is the path relative to
# PROJECT_SOURCE_DIR, the bdir arg is the path relative to PROJECT_BINARY_DIR.
# Lastly, in case there is an autotools version of bfile left over
# from a previous autotools build, PROJECT_SRC_DIR/sdir/bfile is removed.
# Note that everytime cmake is run, the bfile is compared to a newly created
# _cmake_tmp_bfile consistent with the current cmake args.
# Note that the sfile arg does NOT contain the .in suffix.
# ~~~
macro(nrn_configure_dest_src bfile bdir sfile sdir)
  set(infile ${PROJECT_SOURCE_DIR}/${sdir}/${sfile}.in)
  set(bin_dir ${PROJECT_BINARY_DIR}/${bdir})
  file(MAKE_DIRECTORY ${bin_dir})
  execute_process(
    COMMAND sed "s/\#undef *\\(.*\\)/\#cmakedefine \\1 @\\1@/"
    INPUT_FILE ${infile}
    OUTPUT_FILE ${bin_dir}/_cmake_tmp_${bfile}.in)
  configure_file(${bin_dir}/_cmake_tmp_${bfile}.in ${bin_dir}/_cmake_tmp_${bfile} @ONLY)
  execute_process(COMMAND cmp -s ${bin_dir}/_cmake_tmp_${bfile} ${bin_dir}/${bfile} RESULT_VARIABLE result)
  if(result EQUAL 0)
    file(REMOVE ${bin_dir}/_cmake_tmp_${bfile})
  else()
    file(RENAME ${bin_dir}/_cmake_tmp_${bfile} ${bin_dir}/${bfile})
  endif()
  file(REMOVE ${bin_dir}/_cmake_tmp_${bfile}.in)
  set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${infile})
  file(REMOVE "${PROJECT_SOURCE_DIR}/${sdir}/${bfile}")
endmacro()

macro(nrn_configure_file file dir)
  nrn_configure_dest_src(${file} ${dir} ${file} ${dir})
endmacro()

# =============================================================================
# Perform check_include_files and add it to NRN_HEADERS_INCLUDE_LIST if exist
# =============================================================================
macro(nrn_check_include_files filename variable)
  check_include_files(${filename} ${variable})
  if(${variable})
    list(APPEND NRN_HEADERS_INCLUDE_LIST ${filename})
  endif()
endmacro()

# =============================================================================
# Perform check_symbol_exists using NRN_HEADERS_INCLUDE_LIST if empty header_list
# =============================================================================
macro(nrn_check_symbol_exists name header_list variable)
  if("${header_list}" STREQUAL "")
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
  if("${header_list}" STREQUAL "")
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
# Copy file from source to destination in noclobber mode (i.e. no overwrite)
# =============================================================================
macro(nrn_copy_file_without_overwrite source destination)
  execute_process(COMMAND cp -n ${source} ${destination})
endmacro()

# =============================================================================
# Copy file from source to destination only if different
# =============================================================================
macro(nrn_copy_file_if_different source destination)
  configure_file(${source} ${destination} COPYONLY)
endmacro()

# =============================================================================
# Set string with double quotes
# =============================================================================
macro(nrn_set_string variable value)
  set(${variable} \"${value}\")
endmacro()

# =============================================================================
# Set var to to dos path format
# =============================================================================
macro(dospath path var)
  # file(TO_NATIVE_PATH does not convert / to \ for us in msys2.
  string(REPLACE "/" "\\" var1 "${path}")
  set(${var} ${var1})
endmacro()

# =============================================================================
# Given list of file names, find their path in project source tree
# =============================================================================
macro(nrn_find_project_files list_name)
  foreach(name ${ARGN})
    file(GLOB_RECURSE filepath "${PROJECT_SOURCE_DIR}/src/*${name}")
    if(filepath STREQUAL "")
      message(FATAL_ERROR " ${name} not found in ${PROJECT_SOURCE_DIR}/src")
    else()
      list(APPEND ${list_name} ${filepath})
    endif()
  endforeach(name)
endmacro()

# =============================================================================
# Utility macro to print all matching CMake variables
# =============================================================================
# example usage : nrn_print_matching_variables("[Mm][Pp][Ii]")
macro(nrn_print_matching_variables prefix_regex)
  get_cmake_property(variable_names VARIABLES)
  list(SORT variable_names)
  foreach(variable ${variable_names})
    if(variable MATCHES "^${prefix_regex}")
      message(NOTICE " ${variable} ${${variable}}")
    endif()
  endforeach()
endmacro()

# =============================================================================
# Run nocmodl to convert NMODL to C
# =============================================================================
macro(nocmodl_mod_to_c modfile_basename)
  add_custom_command(
    OUTPUT ${modfile_basename}.c
    COMMAND ${CMAKE_COMMAND} -E env "MODLUNIT=${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib"
            ${PROJECT_BINARY_DIR}/bin/nocmodl ${modfile_basename}.mod
    COMMAND sed "'s/_reg()/_reg_()/'" ${modfile_basename}.c > ${modfile_basename}.c.tmp
    COMMAND mv ${modfile_basename}.c.tmp ${modfile_basename}.c
    DEPENDS ${PROJECT_BINARY_DIR}/bin/nocmodl ${modfile_basename}.mod
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/src/nrniv)
endmacro()
