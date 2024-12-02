find_package(Git QUIET)

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
