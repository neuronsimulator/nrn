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
  if(APPLE
     AND NRN_ENABLE_MPI
     AND NRN_ENABLE_MPI_DYNAMIC)
    # Temporarily unset CMAKE_OSX_ARCHITECTURES to use host architecture for MPI tests
    set(_temp ${CMAKE_OSX_ARCHITECTURES})
    unset(CMAKE_OSX_ARCHITECTURES CACHE)
    find_package(MPI REQUIRED)
    set(CMAKE_OSX_ARCHITECTURES
        ${_temp}
        CACHE STRING "Target architectures for macOS" FORCE)
  else()
    # Standard MPI detection for non-dynamic MPI or non-macOS platforms
    find_package(MPI REQUIRED)
  endif()
endmacro()

# ~~~
# Macro to append build architecture flags to executable and its source
# files on macOS. Arg is the executable target (e.g., mkgstates).
# ~~~
macro(nrn_buildarch_executable EXECUTABLE_NAME)
  if(APPLE)
    # Get the build architecture (set by the build system)
    set(BUILD_ARCH "${CMAKE_SYSTEM_PROCESSOR}") # Example: "arm64" or "x86_64"
    # Set BUILD_ARCH_FLAGS based on CMAKE_OSX_ARCHITECTURES and BUILD_ARCH
    set(BUILD_ARCH_FLAGS "")
    if(BUILD_ARCH AND NOT "${BUILD_ARCH}" IN_LIST CMAKE_OSX_ARCHITECTURES)
      set(BUILD_ARCH_FLAGS "-arch ${BUILD_ARCH}")
    endif()

    get_target_property(SOURCE_FILES ${EXECUTABLE_NAME} SOURCES)

    # Append BUILD_ARCH_FLAGS to each source file's COMPILE_FLAGS
    if(BUILD_ARCH_FLAGS)
      foreach(SOURCE_FILE IN LISTS SOURCE_FILES)
        get_source_file_property(EXISTING_FLAGS "${SOURCE_FILE}" COMPILE_FLAGS)
        if(EXISTING_FLAGS)
          set(NEW_FLAGS "${EXISTING_FLAGS} ${BUILD_ARCH_FLAGS}")
        else()
          set(NEW_FLAGS "${BUILD_ARCH_FLAGS}")
        endif()
        set_source_files_properties("${SOURCE_FILE}" PROPERTIES COMPILE_FLAGS "${NEW_FLAGS}")
      endforeach()

      # Append BUILD_ARCH_FLAGS to the executable's linker flags
      separate_arguments(ARCH_FLAGS_LIST UNIX_COMMAND "${BUILD_ARCH_FLAGS}")
      target_link_options(${EXECUTABLE_NAME} PRIVATE ${ARCH_FLAGS_LIST})
    endif()

    # Debug output to verify settings
    if(BUILD_ARCH_FLAGS)
      message(
        STATUS
          "nrn_buildarch_executable: Applied BUILD_ARCH_FLAGS=${BUILD_ARCH_FLAGS} to ${EXECUTABLE_NAME} and source files: ${SOURCE_FILES}"
      )
    endif()
  else()
    message(STATUS "nrn_buildarch_executable ignored, only relevant to macOS (APPLE)")
  endif()
endmacro()

# ~~~
# Macro to temporarily set CMAKE_OSX_ARCHITECTURES to universal2 (x86_64;arm64) and restore it.
# Usage:
#   nrn_set_universal2_begin()
#   # Code that needs universal2 (e.g., add_subdirectory)
#   nrn_set_universal2_end()
# Sets universal2 only if CMAKE_SYSTEM_PROCESSOR is not in CMAKE_OSX_ARCHITECTURES.
# ~~~
macro(nrn_set_universal2_begin)
  if(APPLE)
    # Check if system processor is in target architectures
    set(_NRN_NEED_UNIVERSAL2 FALSE)
    if(CMAKE_SYSTEM_PROCESSOR AND NOT "${CMAKE_SYSTEM_PROCESSOR}" IN_LIST CMAKE_OSX_ARCHITECTURES)
      set(_NRN_NEED_UNIVERSAL2 TRUE)
    endif()
    if(_NRN_NEED_UNIVERSAL2)
      # Save original CMAKE_OSX_ARCHITECTURES
      set(_NRN_ORIGINAL_OSX_ARCHITECTURES ${CMAKE_OSX_ARCHITECTURES})
      # Set universal2 architectures
      set(CMAKE_OSX_ARCHITECTURES
          "x86_64;arm64"
          CACHE STRING "Target architectures for macOS" FORCE)
      message(
        STATUS
          "nrn_set_universal2_begin: Set CMAKE_OSX_ARCHITECTURES to x86_64;arm64 (system: ${CMAKE_SYSTEM_PROCESSOR})"
      )
    else()
      message(
        STATUS
          "nrn_set_universal2_begin: Skipped, system processor ${CMAKE_SYSTEM_PROCESSOR} already in CMAKE_OSX_ARCHITECTURES"
      )
    endif()
  endif()
endmacro()

macro(nrn_set_universal2_end)
  if(APPLE AND DEFINED _NRN_ORIGINAL_OSX_ARCHITECTURES)
    # Restore original CMAKE_OSX_ARCHITECTURES
    if(_NRN_ORIGINAL_OSX_ARCHITECTURES)
      set(CMAKE_OSX_ARCHITECTURES
          "${_NRN_ORIGINAL_OSX_ARCHITECTURES}"
          CACHE STRING "Target architectures for macOS" FORCE)
      message(
        STATUS
          "nrn_set_universal2_end: Restored CMAKE_OSX_ARCHITECTURES to ${_NRN_ORIGINAL_OSX_ARCHITECTURES}"
      )
    else()
      unset(CMAKE_OSX_ARCHITECTURES CACHE)
      message(STATUS "nrn_set_universal2_end: Unset CMAKE_OSX_ARCHITECTURES")
    endif()
    unset(_NRN_ORIGINAL_OSX_ARCHITECTURES)
  endif()
endmacro()
