add_library(
  nrngnu STATIC
  ACG.cpp
  Binomial.cpp
  DiscUnif.cpp
  Erlang.cpp
  Geom.cpp
  HypGeom.cpp
  isaac64.cpp
  Isaac64RNG.cpp
  LogNorm.cpp
  MCellRan4RNG.cpp
  mcran4.cpp
  MLCG.cpp
  NegExp.cpp
  Normal.cpp
  nrnisaac.cpp
  nrnran123.cpp
  Poisson.cpp
  Rand.cpp
  Random.cpp
  RndInt.cpp
  RNG.cpp
  Uniform.cpp
  Weibull.cpp)
set_property(TARGET nrngnu PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(nrngnu SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/external/Random123/include")
target_compile_definitions(nrngnu PRIVATE ${NRN_R123_COMPILE_DEFS})
