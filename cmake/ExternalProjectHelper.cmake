find_package(Git QUIET)

# initialize submodule with given path
function(initialize_submodule path)
  if(NOT ${GIT_FOUND})
    message(
      FATAL_ERROR "git not found and ${path} sub-module not cloned (use git clone --recursive)")
  endif()
  message(STATUS "Sub-module : missing ${path} : running git submodule update --init --recursive")
  execute_process(COMMAND git submodule update --init --recursive -- ${path}
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endfunction()

# check for external project and initialize submodule if it is missing
function(add_external_project name)
  find_path(${name}_PATH NAMES CMakeLists.txt PATHS "${PROJECT_SOURCE_DIR}/external/${name}")
  if(NOT EXISTS ${${name}_PATH})
    initialize_submodule(external/${name})
  else()
    message(STATUS "Sub-project : using ${name} from from external/${name}")
  endif()
  add_subdirectory(external/${name})
endfunction()
