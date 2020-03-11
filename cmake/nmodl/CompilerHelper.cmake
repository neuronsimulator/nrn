# minimal check for c++11 compliant gnu compiler
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  if(NOT (GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9))
    message(FATAL_ERROR "${PROJECT_NAME} requires g++ >= 4.9 (for c++11 support)")
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "PGI")
  set(NMODL_PGI_COMPILER TRUE)

  # CMake adds standard complaint PGI flag "-A" which breaks compilation of of spdlog and fmt
  set(CMAKE_CXX11_STANDARD_COMPILE_OPTION --c++11)
  set(CMAKE_CXX14_STANDARD_COMPILE_OPTION --c++14)

  # ~~~
  # PGI enables number of diagnostic messages by default classes which results into thousands of
  # messages specifically for AST. Disable these verbose warnings for now.
  # TODO : fix these warnings from template modification (#272)
  # ~~~
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --diag_suppress 1,82,111,115,177,186,611,997,1097,1625")
endif()
