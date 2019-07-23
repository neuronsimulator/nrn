# =============================================================================
# Compiler specific settings
# =============================================================================
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -undefined dynamic_lookup")
endif()