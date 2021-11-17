# =============================================================================
# Utility sources
# =============================================================================
set(UTIL_SOURCE_FILES
    common_utils.cpp
    common_utils.hpp
    file_library.cpp
    file_library.hpp
    logger.cpp
    logger.hpp
    perf_stat.cpp
    perf_stat.hpp
    string_utils.cpp
    string_utils.hpp
    table_data.cpp
    table_data.hpp
    ${PROJECT_BINARY_DIR}/src/config/config.cpp)

# =============================================================================
# Symbol table library
# =============================================================================

add_library(util_obj OBJECT ${UTIL_SOURCE_FILES})
set_property(TARGET util_obj PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(util STATIC $<TARGET_OBJECTS:util_obj>)
