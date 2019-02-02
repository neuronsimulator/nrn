#=============================================================================
# Utility sources
#=============================================================================
set(UTIL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/common_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perf_stat.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perf_stat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/table_data.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/table_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp
    ${CMAKE_BINARY_DIR}/version.cpp
)

#=============================================================================
# Symbol table library
#=============================================================================
add_library(util
            STATIC
            ${UTIL_SOURCE_FILES}
)
set_property(TARGET util PROPERTY POSITION_INDEPENDENT_CODE ON)

#=============================================================================
# Files for clang-format
#=============================================================================
set(FILES_FOR_CLANG_FORMAT
    ${UTIL_SOURCE_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/common_utils.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/string_utils.hpp
    ${FILES_FOR_CLANG_FORMAT}
    PARENT_SCOPE
)
