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
macro(nocmodl_mod_to_cpp modfile_basename modfile_compat)
  set(NOCMODL_SED_EXPR "s/_reg()/_reg_()/")
  if(NOT MSVC)
    set(NOCMODL_SED_EXPR "'${NOCMODL_SED_EXPR}'")
  endif()
  set(MODFILE_INPUT_PATH "${PROJECT_SOURCE_DIR}/${modfile_basename}.mod")
  set(MODFILE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/${modfile_basename}.mod")

  # for coreNEURON only
  if(modfile_compat)
    file(READ "${MODFILE_INPUT_PATH}" FILE_CONTENT)
    string(REGEX REPLACE " GLOBAL minf" " RANGE minf" FILE_CONTENT "${FILE_CONTENT}")
    file(WRITE "${MODFILE_OUTPUT_PATH}" "${FILE_CONTENT}")
  else()
    configure_file("${MODFILE_INPUT_PATH}" "${MODFILE_OUTPUT_PATH}" COPYONLY)
  endif()

  get_filename_component(modfile_output_dir "${MODFILE_OUTPUT_PATH}" DIRECTORY)
  get_filename_component(modfile_name "${MODFILE_OUTPUT_PATH}" NAME_WE)
  set(CPPFILE_OUTPUT_PATH "${modfile_output_dir}/${modfile_name}.cpp")

  add_custom_command(
    OUTPUT ${CPPFILE_OUTPUT_PATH}
    COMMAND
      ${CMAKE_COMMAND} -E env "MODLUNIT=${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib"
      "NMODL_PYLIB=${PYTHON_LIBRARY}" "NMODLHOME=${PROJECT_BINARY_DIR}"
      ${NRN_NOCMODL_SANITIZER_ENVIRONMENT} $<TARGET_FILE:${NRN_CODEGENERATOR_TARGET}>
      ${MODFILE_OUTPUT_PATH} ${NRN_NMODL_--neuron} -o ${modfile_output_dir}
    COMMAND sed ${NOCMODL_SED_EXPR} ${CPPFILE_OUTPUT_PATH} > ${CPPFILE_OUTPUT_PATH}.tmp
    COMMAND ${CMAKE_COMMAND} -E copy ${CPPFILE_OUTPUT_PATH}.tmp ${CPPFILE_OUTPUT_PATH}
    COMMAND ${CMAKE_COMMAND} -E remove ${CPPFILE_OUTPUT_PATH}.tmp
    DEPENDS ${NRN_CODEGENERATOR_TARGET} ${MODFILE_INPUT_PATH}
            ${PROJECT_BINARY_DIR}/share/nrn/lib/nrnunits.lib
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/src/nrniv)

endmacro()

# ~~~
# =============================================================================
# Create symbolic links during the install phase
#
# Usage: nrn_install_dir_symlink(source_dir symlink_dir)
#
# Creates a relative symbolic link at `CMAKE_INSTALL_PREFIX/symlink_dir` pointing
# to `source_dir`, executed during `ninja install`. The link is relative to the
# symlink's parent directory (e.g., `x86_64/bin -> ../bin`).
#
# Arguments:
#   source_dir: Path to the target directory (absolute, e.g., `/home/user/bin`,
#               or relative, e.g., `bin`, `neuron/.data/bin`).
#   symlink_dir: Path where the symlink is created (absolute or relative, e.g.,
#                `x86_64/bin`, `neuron/.data/x86_64/bin`).
#
# In NEURON, used to create `install/x86_64/bin -> ../bin` and `install/x86_64/lib -> ../lib`
# for non-wheel builds, or `install/neuron/.data/x86_64/bin -> ../bin` for wheel builds.
#
# The macro ensures the symlink's parent directory (e.g., `install/x86_64`) exists
# and computes the relative path from the symlink's parent to the source directory.
# =============================================================================
# ~~~
macro(nrn_install_dir_symlink source_dir symlink_dir)
  # Create variables for substitution (@arg@ not allowed for CONFIGURE)
  set(src_dir "${source_dir}")
  set(link_dir "${symlink_dir}")
  get_filename_component(parent_symlink_dir "${link_dir}" DIRECTORY)

  # ~~~
  # Define CODE block with placeholders
  # @name@ seems to be the only way to get names declared in this macro
  # to get their proper value during install. ${name} ends up empty.
  # ~~~
  set(code
      [[
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_INSTALL_PREFIX}/@parent_symlink_dir@"
      COMMAND_ECHO STDOUT
    )
    if(IS_ABSOLUTE "@src_dir@")
      set(abs_source_dir "@src_dir@")
    else()
      file(TO_CMAKE_PATH "${CMAKE_INSTALL_PREFIX}/@src_dir@" abs_source_dir)
    endif()
    if(IS_ABSOLUTE "@link_dir@")
      set(abs_symlink_dir "@link_dir@")
    else()
      file(TO_CMAKE_PATH "${CMAKE_INSTALL_PREFIX}/@link_dir@" abs_symlink_dir)
    endif()
    # Compute relative path from symlink's parent to source
    get_filename_component(abs_parent_symlink_dir "${abs_symlink_dir}" DIRECTORY)
    file(RELATIVE_PATH rel_path "${abs_parent_symlink_dir}" "${abs_source_dir}")

    execute_process(
      COMMAND ${CMAKE_COMMAND} -E create_symlink "${rel_path}" "${abs_symlink_dir}"
      COMMAND_ECHO STDOUT
    )
  ]])

  string(CONFIGURE "${code}" configured_code @ONLY)
  install(CODE "${configured_code}")
endmacro()

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

# copy a list of files to the build dir
function(copy_build_list FILE_LIST BUILD_PREFIX)
  foreach(path IN LISTS ${FILE_LIST})
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${path}" "${BUILD_PREFIX}/${path}" COPYONLY)
  endforeach()
endfunction()

# install a list of files to an install dir
function(install_list FILE_LIST INSTALL_PREFIX)
  foreach(file IN LISTS ${FILE_LIST})
    get_filename_component(file_abs "${CMAKE_CURRENT_SOURCE_DIR}/${file}" ABSOLUTE)
    get_filename_component(file_dir "${file}" DIRECTORY)
    install(FILES "${file_abs}" DESTINATION "${INSTALL_PREFIX}/${file_dir}")
  endforeach()
endfunction()

# Replacement for git2nrnversion_h.sh. Add git information to `target` with scope `scope` (PRIVATE,
# PUBLIC, or INTERFACE)
function(add_cpp_git_information target scope)
  find_program(GIT git)
  if(EXISTS "${PROJECT_SOURCE_DIR}/.git" AND GIT)
    execute_process(
      COMMAND "${GIT}" -C "${PROJECT_SOURCE_DIR}" describe
      OUTPUT_VARIABLE GIT_DESCRIBE
      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

    execute_process(
      COMMAND "${GIT}" -C "${PROJECT_SOURCE_DIR}" rev-parse --abbrev-ref HEAD
      OUTPUT_VARIABLE GIT_BRANCH
      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

    execute_process(
      COMMAND "${GIT}" -C "${PROJECT_SOURCE_DIR}" -c log.showSignature=false log --format=%h -n 1
      OUTPUT_VARIABLE GIT_COMMIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

    execute_process(
      COMMAND "${GIT}" -C "${PROJECT_SOURCE_DIR}" -c log.showSignature=false log --format=%cd -n 1
              --date=short
      OUTPUT_VARIABLE GIT_DATE
      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

    execute_process(
      COMMAND "${GIT}" -C "${PROJECT_SOURCE_DIR}" status -s -uno --porcelain
      OUTPUT_VARIABLE GIT_STATUS
      ERROR_QUIET)

    if(GIT_STATUS)
      set(GIT_MODIFIED "+")
    else()
      set(GIT_MODIFIED "")
    endif()

    set(GIT_CHANGESET "${GIT_COMMIT_HASH}${GIT_MODIFIED}")
    set(GIT_DESCRIBE_FULL "${GIT_DESCRIBE}${GIT_MODIFIED}")

  else()
    string(TIMESTAMP BUILD_TIME "%Y-%m-%d-%H:%M:%S")
    set(GIT_DATE "Build Time: ${BUILD_TIME}")
    set(GIT_BRANCH "unknown branch")
    set(GIT_CHANGESET "unknown commit id")
    set(GIT_DESCRIBE "${PROJECT_VERSION}.dev0")
    set(GIT_DESCRIBE_FULL "${GIT_DESCRIBE}")
  endif()

  set(git_def_keys GIT_DATE GIT_BRANCH GIT_CHANGESET GIT_DESCRIBE GIT_DESCRIBE_FULL)

  set(processed_defs)
  foreach(key IN LISTS git_def_keys)
    list(APPEND processed_defs "${key}=\"${${key}}\"")
  endforeach()

  target_compile_definitions(${target} ${scope} ${processed_defs})
endfunction()
