# Common settings
if(CMAKE_VERSION VERSION_LESS 2.8.3)
  get_filename_component(CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} PATH) # WAR bug
endif()

set(GIT_DOCUMENTATION_REPO BBPDocumentation)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/oss)
include(oss/Common)
