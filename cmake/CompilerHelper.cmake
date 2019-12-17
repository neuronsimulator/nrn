# =============================================================================
# Compiler specific settings
# =============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(UNDEFINED_SYMBOLS_IGNORE_FLAG "-undefined dynamic_lookup")
  set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${UNDEFINED_SYMBOLS_IGNORE_FLAG}")
  set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "${UNDEFINED_SYMBOLS_IGNORE_FLAG}")
else()
  set(UNDEFINED_SYMBOLS_IGNORE_FLAG "--unresolved-symbols=ignore-all")
endif()

if(CMAKE_C_COMPILER_ID MATCHES "PGI")
  set(USING_PGI_COMPILER_TRUE "")
  set(USING_PGI_COMPILER_FALSE "#")
else()
  set(USING_PGI_COMPILER_TRUE "#")
  set(USING_PGI_COMPILER_FALSE "")
endif()
