find_package(Git QUIET)

function(add_external_project name)
  find_path(${name}_PATH NAMES CMakeLists.txt PATHS "${PROJECT_SOURCE_DIR}/external/${name}")
  if(NOT EXISTS ${${name}_PATH})
    if(NOT ${GIT_FOUND})
      message(FATAL_ERROR "git not found and ${name} sub-project is not cloned")
    endif()
    message(
      STATUS
        "Sub-project : missing external/${name} : running git submodule update --init --recursive")
    execute_process(COMMAND git submodule update --init --recursive -- external/${name}
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  else()
    message(STATUS "Sub-project : using ${name} from from external/${name}")
  endif()
  add_subdirectory(external/${name})
endfunction()
