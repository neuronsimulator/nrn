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

if(CMAKE_C_COMPILER_ID MATCHES "PGI" OR CMAKE_C_COMPILER_ID MATCHES "NVHPC")
  set(USING_PGI_COMPILER_TRUE "")
  set(USING_PGI_COMPILER_FALSE "#")
  # See https://gitlab.kitware.com/cmake/cmake/-/issues/22168, upper limit of
  # 3.20.3 is based on the current assigned milestone there.
  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20" AND ${CMAKE_VERSION} VERSION_LESS "3.20.3")
    string(APPEND CMAKE_DEPFILE_FLAGS_CXX "-MD<DEP_FILE>")
  endif()
else()
  set(USING_PGI_COMPILER_TRUE "#")
  set(USING_PGI_COMPILER_FALSE "")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "PGI")
  # CMake adds strict standard complaint PGI flag "-A" which breaks
  # compilation of old codes (e.g. spdlog, fmt)
  set(CMAKE_CXX98_STANDARD_COMPILE_OPTION "")
  set(CMAKE_CXX11_STANDARD_COMPILE_OPTION --c++11)
  set(CMAKE_CXX14_STANDARD_COMPILE_OPTION --c++14)
endif()
