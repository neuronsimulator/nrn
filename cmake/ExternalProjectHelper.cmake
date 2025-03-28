find_package(Git QUIET)

if(${GIT_FOUND} AND EXISTS ${CMAKE_SOURCE_DIR}/.git)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} --git-dir=.git describe --all
    RESULT_VARIABLE NOT_A_GIT_REPO
    OUTPUT_QUIET ERROR_QUIET
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
else()
  set(NOT_A_GIT_REPO "NotAGitRepo")
endif()

function(nrn_submodule_file_not_found name_for_message)
  if(NOT_A_GIT_REPO)
    message(
      FATAL_ERROR
        "To build NEURON from source you either need to clone the NEURON Git repository or "
        "download a source code archive that includes Git submodules, such as the "
        "full-src-package-X.Y.Z.tar.gz file in a NEURON release on GitHub (${name_for_message} "
        "could not be found).")
  endif()
endfunction(nrn_submodule_file_not_found)

# initialize submodule with given path
function(nrn_initialize_submodule path)
  cmake_parse_arguments(PARSE_ARGV 1 opt "RECURSIVE;SHALLOW" "" "")
  set(UPDATE_OPTIONS "")
  if(opt_RECURSIVE)
    list(APPEND UPDATE_OPTIONS --recursive)
  endif()
  if(opt_SHALLOW)
    # RHEL7-family distributions ship with an old git that does not support the --depth argument to
    # git submodule update
    if(GIT_VERSION_STRING VERSION_GREATER_EQUAL "1.8.4")
      list(APPEND UPDATE_OPTIONS --depth 1)
    endif()
  endif()
  if(NOT ${GIT_FOUND})
    message(
      FATAL_ERROR "git not found and ${path} sub-module not cloned (use git clone --recursive)")
  endif()
  message(
    STATUS "Sub-module : missing ${path} : running git submodule update ${UPDATE_OPTIONS} --init")
  execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update ${UPDATE_OPTIONS} --init -- ${path}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE ret)
  if(NOT ret EQUAL 0)
    message(FATAL_ERROR "git submodule init failed")
  endif()
endfunction()

# check for external project and initialize submodule if it is missing
function(nrn_add_external_project name)
  cmake_parse_arguments(PARSE_ARGV 1 opt "RECURSIVE;SHALLOW;DISABLE_ADD" "" "")
  find_path(
    ${name}_PATH
    NAMES CMakeLists.txt
    PATHS "${THIRD_PARTY_DIRECTORY}/${name}"
    NO_DEFAULT_PATH)
  if(NOT EXISTS ${${name}_PATH})
    nrn_submodule_file_not_found("${THIRD_PARTY_DIRECTORY}/${name}")
    set(OPTIONS "")
    if(opt_RECURSIVE)
      list(APPEND OPTIONS "RECURSIVE")
    endif()
    if(opt_SHALLOW)
      list(APPEND OPTIONS "SHALLOW")
    endif()
    nrn_initialize_submodule("${THIRD_PARTY_DIRECTORY}/${name}" ${OPTIONS})
  else()
    message(STATUS "Sub-project : using ${name} from from ${THIRD_PARTY_DIRECTORY}/${name}")
  endif()
  if(NOT opt_DISABLE_ADD)
    add_subdirectory("${THIRD_PARTY_DIRECTORY}/${name}")
  endif()
endfunction()
