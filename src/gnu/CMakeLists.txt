add_library(
  nrngnu STATIC
  ACG.cpp
  Binomial.cpp
  DiscUnif.cpp
  Erlang.cpp
  Geom.cpp
  HypGeom.cpp
  LogNorm.cpp
  MLCG.cpp
  NegExp.cpp
  Normal.cpp
  Poisson.cpp
  Random.cpp
  RndInt.cpp
  RNG.cpp
  Uniform.cpp
  Weibull.cpp)
set_property(TARGET nrngnu PROPERTY POSITION_INDEPENDENT_CODE ON)
