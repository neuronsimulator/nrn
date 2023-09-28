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

function(make_nanobind_target target_name pyinc)
  add_library(${target_name} OBJECT ${NB_SOURCE_FILES})
  target_include_directories(${target_name} PUBLIC ${NB_DIR}/include)
  target_include_directories(${target_name} PRIVATE ${NB_DIR}/ext/robin_map/include ${pyinc})
  set_property(TARGET ${target_name} PROPERTY C_VISIBILITY_PRESET hidden)
  set_property(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE True)
endfunction()
