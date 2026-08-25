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

include(CheckCSourceCompiles)

# =============================================================================
# Check if given type exists by compiling code
# =============================================================================
function(nrn_check_type_exists header type default_type variable)
  set(source
      "
    #include <${header}>
    int main() {
      (void)sizeof(${type});
      return 0;
    }")
  check_c_source_compiles("${source}" _my_internal_result)
  if(NOT _my_internal_result)
    set(${variable}
        ${default_type}
        PARENT_SCOPE)
  endif()
endfunction()

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

# Drop FindMPI cache leftovers from a failed configure / cl.exe wrapper guess.
function(nrn_mpi_clear_msvc_cache)
  unset(MPI_C_HEADER_DIR CACHE)
  unset(MPI_CXX_HEADER_DIR CACHE)
  unset(MPI_C_WORKS CACHE)
  unset(MPI_CXX_WORKS CACHE)
  unset(MPI_C_COMPILER_INCLUDE_DIRS CACHE)
  unset(MPI_CXX_COMPILER_INCLUDE_DIRS CACHE)
endfunction()

# Microsoft MPI at C:/msmpi is the Windows prefix (ci/win_install_deps.cmd, ci/win_build_cmake.sh).
# FindMPI's MSMPI guess uses MSMPI_INC / MSMPI_LIB64 only; CMAKE_PREFIX_PATH does not find
# lib/x64/msmpi.lib. The MinGW build already passes FindMPI cache entries. Set the same ones from
# MPI_HOME, CMAKE_PREFIX_PATH, or C:/msmpi when mpi.h and msmpi.lib are there.
function(nrn_windows_prepare_msmpi)
  if(NOT WIN32)
    return()
  endif()
  # MS-MPI has no compiler wrapper. Skip interrogating cl.exe, which can cache the source tree as
  # MPI_*_COMPILER_INCLUDE_DIRS.
  set(MPI_GUESS_LIBRARY_NAME
      MSMPI
      CACHE STRING "MPI implementation to guess on Windows")
  if(MPI_msmpi_LIBRARY AND EXISTS "${MPI_msmpi_LIBRARY}")
    if(NOT MPI_C_ADDITIONAL_INCLUDE_DIRS)
      get_filename_component(_nrn_msmpi_libdir "${MPI_msmpi_LIBRARY}" DIRECTORY)
      get_filename_component(_nrn_msmpi_root "${_nrn_msmpi_libdir}" DIRECTORY)
      get_filename_component(_nrn_msmpi_root "${_nrn_msmpi_root}" DIRECTORY)
      if(EXISTS "${_nrn_msmpi_root}/include/mpi.h")
        set(MPI_C_ADDITIONAL_INCLUDE_DIRS
            "${_nrn_msmpi_root}/include"
            CACHE STRING "MPI C additional include directories" FORCE)
        set(MPI_CXX_ADDITIONAL_INCLUDE_DIRS
            "${_nrn_msmpi_root}/include"
            CACHE STRING "MPI CXX additional include directories" FORCE)
      elseif(EXISTS "${_nrn_msmpi_root}/Include/mpi.h")
        set(MPI_C_ADDITIONAL_INCLUDE_DIRS
            "${_nrn_msmpi_root}/Include"
            CACHE STRING "MPI C additional include directories" FORCE)
        set(MPI_CXX_ADDITIONAL_INCLUDE_DIRS
            "${_nrn_msmpi_root}/Include"
            CACHE STRING "MPI CXX additional include directories" FORCE)
      endif()
    endif()
    nrn_mpi_clear_msvc_cache()
    return()
  endif()

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_nrn_msmpi_libdirs lib/x64 Lib/x64)
  else()
    set(_nrn_msmpi_libdirs lib/x86 Lib/x86)
  endif()

  set(_nrn_msmpi_roots)
  if(MPI_HOME)
    list(APPEND _nrn_msmpi_roots "${MPI_HOME}")
  endif()
  if(DEFINED ENV{MPI_HOME} AND NOT "$ENV{MPI_HOME}" STREQUAL "")
    list(APPEND _nrn_msmpi_roots "$ENV{MPI_HOME}")
  endif()
  foreach(_p IN LISTS CMAKE_PREFIX_PATH)
    list(APPEND _nrn_msmpi_roots "${_p}")
  endforeach()
  list(APPEND _nrn_msmpi_roots "C:/msmpi" "C:/ms-mpi")

  foreach(_root IN LISTS _nrn_msmpi_roots)
    set(_inc "")
    if(EXISTS "${_root}/include/mpi.h")
      set(_inc "${_root}/include")
    elseif(EXISTS "${_root}/Include/mpi.h")
      set(_inc "${_root}/Include")
    endif()
    if(NOT _inc)
      continue()
    endif()
    set(_lib "")
    foreach(_libdir IN LISTS _nrn_msmpi_libdirs)
      if(EXISTS "${_root}/${_libdir}/msmpi.lib")
        set(_lib "${_root}/${_libdir}/msmpi.lib")
        break()
      endif()
    endforeach()
    if(NOT _lib)
      continue()
    endif()
    set(_exec "")
    foreach(_bin "${_root}/bin" "${_root}/Bin")
      if(EXISTS "${_bin}/mpiexec.exe")
        set(_exec "${_bin}/mpiexec.exe")
        break()
      endif()
    endforeach()
    # Same cache entries as ci/win_build_cmake.sh. Unset NOTFOUND leftovers from a previous failed
    # configure in this build dir.
    set(MPI_C_LIB_NAMES
        msmpi
        CACHE STRING "MPI C libraries to link against" FORCE)
    set(MPI_CXX_LIB_NAMES
        msmpi
        CACHE STRING "MPI CXX libraries to link against" FORCE)
    set(MPI_msmpi_LIBRARY
        "${_lib}"
        CACHE FILEPATH "Location of the msmpi library for Microsoft MPI" FORCE)
    set(MPI_C_ADDITIONAL_INCLUDE_DIRS
        "${_inc}"
        CACHE STRING "MPI C additional include directories" FORCE)
    set(MPI_CXX_ADDITIONAL_INCLUDE_DIRS
        "${_inc}"
        CACHE STRING "MPI CXX additional include directories" FORCE)
    if(_exec AND NOT MPIEXEC_EXECUTABLE)
      set(MPIEXEC_EXECUTABLE
          "${_exec}"
          CACHE FILEPATH "Executable for running MPI programs.")
    endif()
    nrn_mpi_clear_msvc_cache()
    message(STATUS "MS-MPI from ${_root} (${_lib})")
    return()
  endforeach()
endfunction()

# ========================================================================
# There is an edge case to 'find_package(MPI REQUIRED)' in that we can still build a universal2
# macos package on an arm64 architecture even if the mpi library has no slice for x86_64.
# ========================================================================
macro(nrn_mpi_find_package)
  nrn_windows_prepare_msmpi()
  if(WIN32)
    # MS-MPI has no compiler wrapper. Do not interrogate cl.exe.
    set(MPI_SKIP_COMPILER_WRAPPER TRUE)
  endif()
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
  elseif(WIN32)
    find_package(MPI)
    if(NOT MPI_FOUND)
      message(
        FATAL_ERROR
          "MPI is required when NRN_ENABLE_MPI=ON, but FindMPI did not find mpi.h / msmpi.lib. "
          "The Windows prefix is C:/msmpi (include/mpi.h and lib/x64/msmpi.lib; SDK, not "
          "runtime-only). Same FindMPI cache entries as ci/win_build_cmake.sh:\n"
          "  -DCMAKE_PREFIX_PATH=C:/msmpi\n"
          "  -DMPI_C_LIB_NAMES=msmpi -DMPI_CXX_LIB_NAMES=msmpi\n"
          "  -DMPI_msmpi_LIBRARY=C:/msmpi/lib/x64/msmpi.lib\n"
          "Or -DMPI_HOME=C:/msmpi. A probe without MPI is -DNRN_ENABLE_MPI=OFF; "
          "that is not the Windows wheel product.")
    endif()
  else()
    find_package(MPI REQUIRED)
  endif()
  # Keep only directories that actually contain mpi.h. FindMPI can assemble MPI_C_INCLUDE_DIRS from
  # compiler-wrapper leftovers (the source tree).
  set(_nrn_mpi_incs)
  foreach(_d IN LISTS MPI_C_INCLUDE_DIRS MPI_INCLUDE_PATH)
    if(_d AND EXISTS "${_d}/mpi.h")
      list(APPEND _nrn_mpi_incs "${_d}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES _nrn_mpi_incs)
  set(MPI_C_INCLUDE_DIRS "${_nrn_mpi_incs}")
  set(MPI_INCLUDE_PATH "${_nrn_mpi_incs}")
  unset(_nrn_mpi_incs)
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
  include("${PROJECT_SOURCE_DIR}/cmake/NrnGitInfo.cmake")
  set(_nrn_git_stamp "${PROJECT_SOURCE_DIR}/cmake/nrn-git-info.cmake")
  nrn_collect_git_info("${PROJECT_SOURCE_DIR}" _nrn_git_ok)
  if(_nrn_git_ok)
    nrn_write_git_info_stamp("${_nrn_git_stamp}")
  elseif(EXISTS "${_nrn_git_stamp}")
    include("${_nrn_git_stamp}")
    message(STATUS "git describe failed; using ${_nrn_git_stamp} (${GIT_CHANGESET})")
  else()
    # Shallow clone, tarball, or a vboxsf worktree with no host stamp.
    string(TIMESTAMP BUILD_TIME "%Y-%m-%d-%H:%M:%S")
    set(GIT_DATE "Build Time: ${BUILD_TIME}")
    set(GIT_BRANCH "unknown branch")
    set(GIT_CHANGESET "unknown commit id")
    set(GIT_DESCRIBE "${PROJECT_VERSION}.dev0")
    set(GIT_DESCRIBE_FULL "${GIT_DESCRIBE}")
    message(STATUS "git describe failed or empty; GIT_DESCRIBE=${GIT_DESCRIBE}")
  endif()

  set(git_def_keys GIT_DATE GIT_BRANCH GIT_CHANGESET GIT_DESCRIBE GIT_DESCRIBE_FULL)

  set(processed_defs)
  foreach(key IN LISTS git_def_keys)
    list(APPEND processed_defs "${key}=\"${${key}}\"")
  endforeach()

  target_compile_definitions(${target} ${scope} ${processed_defs})
endfunction()
