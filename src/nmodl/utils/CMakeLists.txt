# =============================================================================
# Utility library
# =============================================================================
add_library(
  util STATIC
  blame.cpp
  common_utils.cpp
  file_library.cpp
  logger.cpp
  perf_stat.cpp
  string_utils.cpp
  table_data.cpp
  ${PROJECT_BINARY_DIR}/src/nmodl/config/config.cpp)

target_include_directories(util PUBLIC "${PROJECT_BINARY_DIR}/share/nmodl")
set_property(TARGET util PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(util PUBLIC fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)

if(NRN_ENABLE_BACKTRACE)
  add_backward(util)
  target_compile_definitions(util PUBLIC NRN_USE_BACKWARD=1)
endif()
