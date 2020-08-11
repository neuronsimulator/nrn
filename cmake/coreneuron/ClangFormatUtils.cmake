# =============================================================================
# Copyright (C) 2016-2020 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

string(REPLACE " " ";" FILES_TO_FORMAT ${SOURCE_FILES})

foreach(SRC_FILE ${FILES_TO_FORMAT})
  execute_process(COMMAND ${CLANG_FORMAT_EXECUTABLE} -i -style=file -fallback-style=none
                          ${SRC_FILE})
endforeach()
