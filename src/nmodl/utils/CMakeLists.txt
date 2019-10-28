# =============================================================================
# Utility sources
# =============================================================================
set(UTIL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/common_utils.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/string_utils.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perf_stat.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perf_stat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/table_data.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/table_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp
    ${PROJECT_BINARY_DIR}/src/config/config.cpp)

# =============================================================================
# Symbol table library
# =============================================================================

add_library(util_obj OBJECT ${UTIL_SOURCE_FILES})
set_property(TARGET util_obj PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(util STATIC $<TARGET_OBJECTS:util_obj>)
