# =============================================================================
# Utility library
# =============================================================================
add_library(
  util STATIC
  common_utils.cpp
  file_library.cpp
  logger.cpp
  perf_stat.cpp
  string_utils.cpp
  table_data.cpp
  ${PROJECT_BINARY_DIR}/src/config/config.cpp)
set_property(TARGET util PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(util PUBLIC fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog_header_only)
