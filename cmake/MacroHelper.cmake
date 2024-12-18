# =============================================================================
# Helper functions used in the project
# =============================================================================

include(CheckIncludeFileCXX)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)
include(CMakeParseArguments)

set(CMAKE_REQUIRED_QUIET TRUE)

# =============================================================================
# Check if directory related to DIR exists by compiling code
# =============================================================================
macro(nrn_check_dir_exists HEADER VARIABLE)
  # code template to check existence of DIR
  string(
    CONCAT CONFTEST_DIR_TPL
           "#include <sys/types.h>\n"
           "#include <@dir_header@>\n"
           "int main () {\n"
           "  if ((DIR *) 0)\n"
           "    return 0\;\n"
           "  return 0\;\n"
           "}\n")
  # first get header file
  check_include_files(${HEADER} HAVE_HEADER)
  if(${HAVE_HEADER})
    # if header is found, create a code from template
    string(REPLACE "@dir_header@" ${HEADER} CONFTEST_DIR "${CONFTEST_DIR_TPL}")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/conftest.cpp" ${CONFTEST_DIR})
    # try to compile
    try_compile(${VARIABLE} "${CMAKE_CURRENT_SOURCE_DIR}"
                "${CMAKE_CURRENT_SOURCE_DIR}/conftest.cpp")
    file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/conftest.cpp")
  endif()
endmacro()

# =============================================================================
# Check if given type exists by compiling code
# =============================================================================
macro(nrn_check_type_exists HEADER TYPE DEFAULT_TYPE VARIABLE)
  # code template to check existence of specific type
  string(
    CONCAT CONFTEST_TYPE_TPL
           "#include <@header@>\n"
           "int main () {\n"
           "  if (sizeof (@type@))\n"
           "    return 0\;\n"
           "  return 0\;\n"
           "}\n")
  string(REPLACE "@header@" ${HEADER} CONFTEST_TYPE "${CONFTEST_TYPE_TPL}")
  string(REPLACE "@type@" ${TYPE} CONFTEST_TYPE "${CONFTEST_TYPE}")
  file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/conftest.cpp ${CONFTEST_TYPE})

  try_compile(MY_RESULT_VAR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/conftest.cpp)
  if(NOT ${MY_RESULT_VAR})
    set(${VARIABLE} ${DEFAULT_TYPE})
  endif()
  file(REMOVE "conftest.cpp")
endmacro()

# =============================================================================
# Perform check_include_files and add it to NRN_HEADERS_INCLUDE_LIST if exist Passing an optional
# CXX will call check_include_files_cxx instead.
# =============================================================================
macro(nrn_check_include_files filename variable)
  set(options CXX)
  cmake_parse_arguments(nrn_check_include_files "${options}" "" "" ${ARGN})
  if(${nrn_check_include_files_CXX})
    check_include_file_cxx(${filename} ${variable})
  else()
    check_include_files(${filename} ${variable})
  endif()
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
macro(nocmodl_mod_to_cpp modfile_basename modfile_subdir)
  set(NOCMODL_SED_EXPR "s/_reg()/_reg_()/")
  if(NOT MSVC)
    set(NOCMODL_SED_EXPR "'${NOCMODL_SED_EXPR}'")
  endif()
  set(REMOVE_CMAKE_COMMAND "rm")
  if(CMAKE_VERSION VERSION_LESS "3.17")
    set(REMOVE_CMAKE_COMMAND "remove")
  endif()
  string(REPLACE "/" ";" path_parts "${modfile_basename}")

  # Create a variable to hold the base path (all but the last part)
  set(base_path "")
  list(LENGTH path_parts path_length)
  math(EXPR last_index "${path_length} - 1")

  # Append all parts except the last one to base_path
  foreach(i RANGE 0 ${last_index})
    list(GET path_parts ${i} part)
    if(${i} LESS ${last_index})
      if(base_path STREQUAL "")
        set(base_path "${part}")
      else()
        set(base_path "${base_path}/${part}")
      endif()
    endif()
  endforeach()

  # Append the custom part and the last part
  list(GET path_parts ${last_index} last_part)
  set(modfile_basename_real "${base_path}/${modfile_subdir}/${last_part}")

  get_filename_component(modfile_output_dir ${PROJECT_SOURCE_DIR}/${modfile_basename_real}.mod
                         DIRECTORY)
  add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/${modfile_basename}.cpp
    COMMAND
      ${CMAKE_COMMAND} -E env "MODLUNIT=${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib"
      ${NRN_NOCMODL_SANITIZER_ENVIRONMENT} $<TARGET_FILE:${NRN_CODEGENERATOR_TARGET}>
      ${PROJECT_SOURCE_DIR}/${modfile_basename_real}.mod ${NRN_NMODL_--neuron} -o
      ${modfile_output_dir}
    COMMAND sed ${NOCMODL_SED_EXPR} ${PROJECT_SOURCE_DIR}/${modfile_basename_real}.cpp >
            ${PROJECT_BINARY_DIR}/${modfile_basename}.cpp
    COMMAND ${CMAKE_COMMAND} -E ${REMOVE_CMAKE_COMMAND}
            ${PROJECT_SOURCE_DIR}/${modfile_basename_real}.cpp
    DEPENDS ${NRN_CODEGENERATOR_TARGET} ${PROJECT_SOURCE_DIR}/${modfile_basename_real}.mod
            ${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/src/nrniv)
endmacro()

# =============================================================================
# Create symbolic links
# =============================================================================
macro(nrn_install_dir_symlink source_dir symlink_dir)
  # make sure to have directory path exist upto parent dir
  get_filename_component(parent_symlink_dir ${symlink_dir} DIRECTORY)
  install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${parent_symlink_dir})")
  # create symbolic link
  install(
    CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${source_dir} ${symlink_dir})")
endmacro(nrn_install_dir_symlink)

# ========================================================================
# There is an edge case to 'find_package(MPI REQUIRED)' in that we can still build a universal2
# macos package on an arm64 architecture even if the mpi library has no slice for x86_64.
# ========================================================================
macro(nrn_mpi_find_package)
  if("arm64" IN_LIST CMAKE_OSX_ARCHITECTURES
     AND "x86_64" IN_LIST CMAKE_OSX_ARCHITECTURES
     AND NRN_ENABLE_MPI_DYNAMIC)
    set(_temp ${CMAKE_OSX_ARCHITECTURES})
    unset(CMAKE_OSX_ARCHITECTURES CACHE)
    find_package(MPI REQUIRED)
    set(CMAKE_OSX_ARCHITECTURES
        ${_temp}
        CACHE INTERNAL "" FORCE)
    set(NRN_UNIVERSAL2_BUILD ON)
  else()
    find_package(MPI REQUIRED)
  endif()
endmacro()
