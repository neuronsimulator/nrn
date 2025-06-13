add_library(
  nrngnu STATIC
  Binomial.cpp
  DiscUnif.cpp
  Erlang.cpp
  Geom.cpp
  HypGeom.cpp
  LogNorm.cpp
  MCellRan4RNG.cpp
  mcran4.cpp
  NegExp.cpp
  Normal.cpp
  NrnRandom123RNG.cpp
  nrnran123.cpp
  Poisson.cpp
  Rand.cpp
  Random.cpp
  Uniform.cpp
  Weibull.cpp)
set_property(TARGET nrngnu PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(nrngnu SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/external/Random123/include")
target_compile_definitions(nrngnu PRIVATE ${NRN_R123_COMPILE_DEFS})
