add_library(
  nrngnu STATIC
  Geom.cpp
  HypGeom.cpp
  MCellRan4RNG.cpp
  mcran4.cpp
  NrnRandom123RNG.cpp
  nrnran123.cpp
  Rand.cpp
  Random.cpp
  RNG.cpp)
set_property(TARGET nrngnu PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(nrngnu SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/external/Random123/include")
target_compile_definitions(nrngnu PRIVATE ${NRN_R123_COMPILE_DEFS})
