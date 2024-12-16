# =============================================================================
# Support for building NMODL without python library
# =============================================================================
# ~~~
# When NMODL is built under environment  (e.g. manylinux) without python-dev
# package i.e. libpython.so, we have to build extension and binaries by
# ignoring undefned symbols. The python library will be loaded later on target
# system.
# ~~~

# use same variable name as NEURON as it won't be user option
set(LINK_AGAINST_PYTHON
    TRUE
    CACHE BOOL "Disable linking to python library")

mark_as_advanced(LINK_AGAINST_PYTHON)

# Flags for ignoring undefined symbols for wheel
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(UNDEFINED_SYMBOLS_IGNORE_FLAG "-Wl,-undefined,dynamic_lookup")
else()
  set(UNDEFINED_SYMBOLS_IGNORE_FLAG "-Wl,--unresolved-symbols=ignore-all")
endif()

if(NOT LINK_AGAINST_PYTHON)
  string(APPEND CMAKE_EXE_LINKER_FLAGS " ${UNDEFINED_SYMBOLS_IGNORE_FLAG}")
  set(NMODL_WRAPPER_LIBS pyembed dl)
else()
  set(NMODL_WRAPPER_LIBS pyembed pywrapper dl)
endif()
