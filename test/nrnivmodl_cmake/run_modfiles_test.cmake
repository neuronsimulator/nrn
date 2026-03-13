# Ensure cleanup even if something fails
function(cleanup)
  message(STATUS "Cleaning up ${TEST_DIR}")
  file(REMOVE_RECURSE "${TEST_DIR}")
endfunction()

cleanup()

message(STATUS "Mod files: ${MOD_FILES}")
# Due to CMake not being a useful language, and having only a string type, we need to do some
# escaping in order to pass things along to the CLI
string(REPLACE " " "\ " MOD_FILES "${MOD_FILES}")
string(REPLACE ";" "\;" MOD_FILES "${MOD_FILES}")

# Configure
if(CMAKE_CUDA_COMPILER)
  # One would figure saving the below into a variable, and only appending the CUDA stuff if
  # necessary, would work, but alas, due to quoting rules, I can't figure out the magic combination
  # of those that actually work, so we just duplicate the command string verbatim.
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_BINARY_DIR}/build_cmake" -B "${TEST_DIR}"
      -DMOD_FILES=${MOD_FILES} -DCMAKE_BUILD_TYPE=Debug -DCORENEURON=${CORENEURON}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_CUDA_COMPILER=${CMAKE_CUDA_COMPILER}
    RESULT_VARIABLE res_configure)
else()
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_BINARY_DIR}/build_cmake" -B "${TEST_DIR}"
      -DMOD_FILES=${MOD_FILES} -DCMAKE_BUILD_TYPE=Debug -DCORENEURON=${CORENEURON}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    RESULT_VARIABLE res_configure)
endif()

if(NOT res_configure EQUAL 0)
  cleanup()
  message(FATAL_ERROR "Configure failed")
endif()

# Build
set(BUILD_COMMAND "${CMAKE_COMMAND}" --build "${TEST_DIR}" --parallel -v)
execute_process(COMMAND ${BUILD_COMMAND} RESULT_VARIABLE res_build)
if(NOT res_build EQUAL 0)
  message(FATAL_ERROR "Build failed")
endif()

# Run executable
set(EXE_COMMAND "${TEST_DIR}/special" -nopython -nobanner -nogui -c "quit()")
execute_process(COMMAND ${EXE_COMMAND} RESULT_VARIABLE res_run)
if(NOT res_run EQUAL 0)
  cleanup()
  message(FATAL_ERROR "Executable failed")
endif()

# Cleanup at the end
cleanup()
