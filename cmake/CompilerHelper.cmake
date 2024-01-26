# =============================================================================
# Compiler specific settings
# =============================================================================
if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR NRN_MACOS_BUILD)
  set(UNDEFINED_SYMBOLS_IGNORE_FLAG "-undefined dynamic_lookup")
  string(APPEND CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS " ${UNDEFINED_SYMBOLS_IGNORE_FLAG}")
  string(APPEND CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS " ${UNDEFINED_SYMBOLS_IGNORE_FLAG}")
else()
  set(UNDEFINED_SYMBOLS_IGNORE_FLAG "--unresolved-symbols=ignore-all")
endif()

if(CMAKE_C_COMPILER_ID MATCHES "PGI" OR CMAKE_C_COMPILER_ID MATCHES "NVHPC")
  set(NRN_HAVE_NVHPC_COMPILER ON)
  # See https://gitlab.kitware.com/cmake/cmake/-/issues/22168, upper limit of 3.20.3 is based on the
  # current assigned milestone there.
  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20" AND ${CMAKE_VERSION} VERSION_LESS "3.20.3")
    string(APPEND CMAKE_DEPFILE_FLAGS_CXX "-MD<DEP_FILE>")
  endif()

  # CMake versions <3.19 used to add -A when using NVHPC/PGI, which makes the compiler excessively
  # pedantic. See https://gitlab.kitware.com/cmake/cmake/-/issues/20997. Also, stdinit.h include
  # behaviour is different with -A (ANSI C++) and result into an error mentioned in
  # https://github.com/neuronsimulator/nrn/issues/2563
  if(CMAKE_VERSION VERSION_LESS 3.19)
    list(REMOVE_ITEM CMAKE_CXX17_STANDARD_COMPILE_OPTION -A)
  endif()

  if(${CMAKE_C_COMPILER_VERSION} VERSION_GREATER_EQUAL 20.7)
    # https://forums.developer.nvidia.com/t/many-all-diagnostic-numbers-increased-by-1-from-previous-values/146268/3
    # changed the numbering scheme in newer versions. The following list is from a clean start 16
    # August 2021. It would clearly be nicer to apply these suppressions only to relevant files.
    # Examples of the suppressed warnings are given below.
    # ~~~
    # "src/nrniv/nvector_nrnserial_ld.cpp", warning #47-D: incompatible redefinition of macro "..."
    # "src/nmodl/kinetic.cpp", warning #111-D: statement is unreachable
    # "src/nmodl/parsact.cpp", warning #128-D: loop is not reachable
    # "src/modlunit/units.cpp", warning #170-D: pointer points outside of underlying object
    # "src/nrnpython/grids.cpp", warning #174-D: expression has no effect
    # "src/nmodl/nocpout.cpp", warning #177-D: variable "j" was declared but never referenced
    # "src/nrniv/partrans.cpp", warning #186-D: pointless comparison of unsigned integer with zero
    # "src/nrnpython/rxdmath.cpp", warning #541-D: allowing all exceptions is incompatible with previous function
    # "src/nmodl/nocpout.cpp", warning #550-D: variable "sion" was set but never used
    # "src/gnu/neuron_gnu_builtin.h", warning #816-D: type qualifier on return type is meaningless"
    # "src/modlunit/consist.cpp", warning #2465-D: conversion from a string literal to "char *" is deprecated
    # ~~~
    list(APPEND NRN_COMPILE_FLAGS --diag_suppress=1,47,111,128,170,174,177,186,541,550,816,2465)
  endif()
  list(APPEND NRN_COMPILE_FLAGS -noswitcherror)
  list(APPEND NRN_LINK_FLAGS -noswitcherror)
  if(${CMAKE_C_COMPILER_VERSION} VERSION_GREATER_EQUAL 21.11)
    # Random123 does not play nicely with NVHPC 21.11+'s detection of ABM features, see:
    # https://github.com/BlueBrain/CoreNeuron/issues/724 and
    # https://github.com/DEShawResearch/random123/issues/6.
    list(APPEND NRN_R123_COMPILE_DEFS R123_USE_INTRIN_H=0)
  endif()
else()
  set(NRN_HAVE_NVHPC_COMPILER OFF)
endif()
