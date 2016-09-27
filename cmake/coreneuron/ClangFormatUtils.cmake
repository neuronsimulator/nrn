string(REPLACE " " ";" FILES_TO_FORMAT ${SOURCE_FILES})

FOREACH(SRC_FILE ${FILES_TO_FORMAT})
    execute_process(COMMAND ${CLANG_FORMAT_EXECUTABLE} -i -style=file -fallback-style=none ${SRC_FILE})
ENDFOREACH()
