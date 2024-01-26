add_library(
  nrngnu STATIC
  Binomial.cpp
  DiscUnif.cpp
  Erlang.cpp
  Geom.cpp
  HypGeom.cpp
  LogNorm.cpp
  NegExp.cpp
  Normal.cpp
  nrnran123.cpp
  NrnRandom123RNG.cpp
  Poisson.cpp
  Rand.cpp
  Random.cpp
  RndInt.cpp
  RNG.cpp
  Uniform.cpp
  Weibull.cpp)
set_property(TARGET nrngnu PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(nrngnu SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/external/Random123/include")
