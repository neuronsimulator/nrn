# minimal check for c++11 compliant gnu compiler
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  if(NOT (GCC_VERSION VERSION_GREATER 4.8.2 OR GCC_VERSION VERSION_EQUAL 4.8.2))
    message(FATAL_ERROR "${PROJECT_NAME} requires g++ >= 4.8.2 (for C++11 support)")
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "PGI" OR CMAKE_CXX_COMPILER_ID MATCHES "NVHPC")
  set(NMODL_PGI_COMPILER TRUE)

  # CMake adds standard complaint PGI flag "-A" which breaks compilation of of spdlog and fmt
  set(CMAKE_CXX11_STANDARD_COMPILE_OPTION --c++11)
  set(CMAKE_CXX14_STANDARD_COMPILE_OPTION --c++14)

  # ~~~
  # PGI enables number of diagnostic messages by default classes which results into thousands of
  # messages specifically for AST. Disable these verbose warnings for now.
  # TODO : fix these warnings from template modification (#272)
  # ~~~
  if(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 20.7)
    set(NMODL_COMPILER_WARNING_SUPPRESSIONS --diag_suppress=1,82,111,115,177,186,611,997,1097,1625)
  else()
    # https://forums.developer.nvidia.com/t/many-all-diagnostic-numbers-increased-by-1-from-previous-values/146268/3
    # changed the numbering scheme in newer versions. The following list is from a clean start 13
    # August 2021. It would clearly be nicer to apply these suppressions only to relevant files.
    # Examples of the suppressed warnings are given below.
    # ~~~
    # "ext/spdlog/include/spdlog/fmt/fmt.h", warning #1-D: last line of file ends without a newline
    # "ext/fmt/include/fmt/format.h", warning #111-D: statement is unreachable
    # "ext/json/json.hpp", warning #186-D: pointless comparison of unsigned integer with zero
    # "src/ast/all.hpp", warning #998-D: function "..." is hidden by "..." -- virtual function override intended?
    # "ext/spdlog/include/spdlog/fmt/bundled/format.h", warning #1098-D: unknown attribute "fallthrough"
    # "ext/pybind11/include/pybind11/detail/common.h", warning #1626-D: routine is both "inline" and "noinline"
    # ~~~
    # The following warnings do not seem to be suppressible with --diag_suppress:
    # ~~~
    # "src/codegen/codegen_cuda_visitor.cpp", NVC++-W-0277-Cannot inline function - data type mismatch
    # "nvc++IkWUbMugiSgNH.s: Warning: stand-alone `data16' prefix
    # ~~~
    set(NMODL_COMPILER_WARNING_SUPPRESSIONS --diag_suppress=1,111,186,998,1098,1626)
    # There are a few more warnings produced by the unit test infrastructure.
    # ~~~
    # "test/unit/visitor/constant_folder.cpp", warning #177-D: variable "..." was declared but never referenced
    # ~~~
    set(NMODL_TESTS_COMPILER_WARNING_SUPPRESSIONS --diag_suppress=177)
  endif()
endif()
