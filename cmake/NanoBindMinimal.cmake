# =============================================================================
# minimal nanobind interface
# =============================================================================

set(NB_DIR ${PROJECT_SOURCE_DIR}/external/nanobind)

function(make_nanobind_target TARGET_NAME PYINC)
  add_library(${TARGET_NAME} STATIC ${NB_DIR}/src/nb_combined.cpp)
  target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${NB_DIR}/include)
  target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${NB_DIR}/ext/robin_map/include ${PYINC})
  if(MSVC)
    # Do not complain about vsnprintf
    target_compile_definitions(${TARGET_NAME} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
  else()
    # Generally needed to handle type punning in Python code
    target_compile_options(${TARGET_NAME} PRIVATE -fno-strict-aliasing)
  endif()
  target_compile_features(${TARGET_NAME} PUBLIC cxx_std_17)
  set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE True)
endfunction()
