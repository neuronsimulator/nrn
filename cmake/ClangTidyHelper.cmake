if(CMAKE_VERSION VERSION_GREATER "3.5")
  set(ENABLE_CLANG_TIDY
      OFF
      CACHE BOOL "Add clang-tidy automatically to builds")
  if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
      message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
      set(CLANG_TIDY_CHECKS
          "-*,modernize-*,readability-*,performance-*,cppcoreguidelines-*,clang-analyzer-core*,google-*"
      )
      set(CMAKE_CXX_CLANG_TIDY
          "${CLANG_TIDY_EXE};-checks=${CLANG_TIDY_CHECKS};-fix;-header-filter='${CMAKE_SOURCE_DIR}/*'"
          CACHE STRING "" FORCE)
    else()
      message(AUTHOR_WARNING "clang-tidy not found!")
      set(CMAKE_CXX_CLANG_TIDY
          ""
          CACHE STRING "" FORCE) # delete it
    endif()
  endif()
endif()
