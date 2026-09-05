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
target_include_directories(nrngnu PRIVATE "${PROJECT_SOURCE_DIR}/src/oc")
target_include_directories(nrngnu SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/external/Random123/include")
target_compile_definitions(nrngnu PRIVATE ${NRN_R123_COMPILE_DEFS})
# nrnran123_* live here. WINDOWS_EXPORT_ALL_SYMBOLS does not scan a static .lib; NRN_DLLSYM is
# dllexport only with NRN_DLL_EXPORTS (MSVC).
if(NRN_LIBRARY_TYPE STREQUAL "SHARED")
  target_compile_definitions(nrngnu PRIVATE NRN_DLL NRN_DLL_EXPORTS)
endif()
