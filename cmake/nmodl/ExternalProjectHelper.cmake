find_package(Git QUIET)

set(THIRD_PARTY_DIRECTORY
    "${NMODL_PROJECT_SOURCE_DIR}/3rdparty"
    CACHE PATH "The path were all the 3rd party projects can be found")

# initialize submodule with given path
function(initialize_submodule path)
  if(NOT ${GIT_FOUND})
    message(
      FATAL_ERROR "git not found and ${path} sub-module not cloned (use git clone --recursive)")
  endif()
  message(STATUS "Sub-module : missing ${path}: running git submodule update --init")
  execute_process(COMMAND git submodule update --init -- ${path}
                  WORKING_DIRECTORY ${NMODL_PROJECT_SOURCE_DIR})
endfunction()

# check for external project and initialize submodule if it is missing
function(add_external_project name)
  find_path(
    ${name}_PATH
    NAMES CMakeLists.txt
    PATHS "${THIRD_PARTY_DIRECTORY}/${name}")
  if(NOT EXISTS ${${name}_PATH})
    initialize_submodule("${THIRD_PARTY_DIRECTORY}/${name}")
  else()
    message(STATUS "Sub-project : using ${name} from \"${THIRD_PARTY_DIRECTORY}/${name}\"")
  endif()
  if(${ARGC} GREATER 1)
    if(${ARGV2})
      add_subdirectory("${THIRD_PARTY_DIRECTORY}/${name}")
    endif()
  else()
    add_subdirectory("${THIRD_PARTY_DIRECTORY}/${name}")
  endif()
endfunction()
