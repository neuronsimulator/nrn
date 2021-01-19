# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# coreneuron-config.cmake - package configuration file

get_filename_component(CONFIG_PATH "${CMAKE_CURRENT_LIST_FILE}" PATH)

find_path(CORENEURON_INCLUDE_DIR "coreneuron/coreneuron.h" HINTS "${CONFIG_PATH}/../../include")
find_path(
  CORENEURON_LIB_DIR
  NAMES libcoreneuron.a libcoreneuron.so libcoreneuron.dylib
  HINTS "${CONFIG_PATH}/../../lib")

include(${CONFIG_PATH}/coreneuron.cmake)
