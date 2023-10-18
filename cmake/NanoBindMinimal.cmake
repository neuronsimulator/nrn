# =============================================================================
# minimal nanobind interface
# =============================================================================

set(NB_DIR ${PROJECT_SOURCE_DIR}/external/nanobind)

set(NB_SOURCE_FILES
    ${NB_DIR}/src/common.cpp
    ${NB_DIR}/src/error.cpp
    ${NB_DIR}/src/nb_internals.cpp
    ${NB_DIR}/src/nb_static_property.cpp
    ${NB_DIR}/src/nb_type.cpp
    ${NB_DIR}/src/nb_func.cpp
    ${NB_DIR}/src/implicit.cpp)

function(make_nanobind_target TARGET_NAME PYINC)
  add_library(${TARGET_NAME} OBJECT ${NB_SOURCE_FILES})
  target_include_directories(${TARGET_NAME} PUBLIC ${NB_DIR}/include)
  target_include_directories(${TARGET_NAME} PRIVATE ${NB_DIR}/ext/robin_map/include ${PYINC})
  target_compile_options(${TARGET_NAME} PRIVATE -fno-strict-aliasing)
  target_compile_definitions(${TARGET_NAME} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
  target_compile_features(${TARGET_NAME} PUBLIC cxx_std_17)
  set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE True)
  set_property(TARGET ${TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endfunction()
