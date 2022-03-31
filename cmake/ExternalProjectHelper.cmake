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

# initialize submodule with given path
function(nrn_initialize_submodule path)
  if(NOT ${GIT_FOUND})
    message(
      FATAL_ERROR "git not found and ${path} sub-module not cloned (use git clone --recursive)")
  endif()
  message(STATUS "Sub-module : missing ${path} : running git submodule update --init")
  execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init -- ${path}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE ret)
  if(ret EQUAL "1")
    message(FATAL_ERROR "git submodule init failed")
  endif()
endfunction()

# check for external project and initialize submodule if it is missing
function(nrn_add_external_project name)
  find_path(
    ${name}_PATH
    NAMES CMakeLists.txt
    PATHS "${THIRD_PARTY_DIRECTORY}/${name}")
  if(NOT EXISTS ${${name}_PATH})
    if(NOT_A_GIT_REPO)
      message(
        FATAL_ERROR "Looks like you are building from source. Git needed for ${name} feature.")
    endif()
    nrn_initialize_submodule("${THIRD_PARTY_DIRECTORY}/${name}")
  else()
    message(STATUS "Sub-project : using ${name} from from ${THIRD_PARTY_DIRECTORY}/${name}")
  endif()
  # if second argument is passed and if it's OFF then skip add_subdirectory
  if(${ARGC} GREATER 1)
    if(${ARGV2})
      add_subdirectory("${THIRD_PARTY_DIRECTORY}/${name}")
    endif()
  else()
    add_subdirectory("${THIRD_PARTY_DIRECTORY}/${name}")
  endif()
endfunction()
