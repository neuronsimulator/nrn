# =============================================================================
# minimal nanobind interface
# =============================================================================

set(NB_DIR "external/nanobind")

add_library(nanobind INTERFACE)
target_sources(
  nanobind
  INTERFACE ${NB_DIR}/src/common.cpp
            ${NB_DIR}/src/error.cpp
            ${NB_DIR}/src/nb_internals.cpp
            ${NB_DIR}/src/nb_static_property.cpp
            ${NB_DIR}/src/nb_type.cpp
            ${NB_DIR}/src/nb_func.cpp
            ${NB_DIR}/src/implicit.cpp)

target_include_directories(nanobind INTERFACE ${NB_DIR}/include ${NB_DIR}/ext/robin_map/include)
