# =============================================================================
# Compiler specific settings
# =============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR NRN_MACOS_BUILD)
  set(UNDEFINED_SYMBOLS_IGNORE_FLAG "-undefined dynamic_lookup")
  string(APPEND CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS " ${UNDEFINED_SYMBOLS_IGNORE_FLAG}")
  string(APPEND CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS " ${UNDEFINED_SYMBOLS_IGNORE_FLAG}")
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

if(CMAKE_CXX_COMPILER_ID MATCHES "PGI")
  # CMake adds strict standard complaint PGI flag "-A" which breaks
  # compilation of old codes (e.g. python2 files, spdlog, fmt)
  set(CMAKE_CXX98_STANDARD_COMPILE_OPTION "")
  set(CMAKE_CXX11_STANDARD_COMPILE_OPTION --c++11)
  set(CMAKE_CXX14_STANDARD_COMPILE_OPTION --c++14)
endif()
