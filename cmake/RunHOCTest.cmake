# =============================================================================
# ~~~
# Execute nrniv to generate an output file and then compare the output file to
# a reference file
#
# Arguments:
#   - executable: nrniv executable
#   - exec_arg: executable arguments
#   - work_dir: working directory
#   - out_file: output file generated by the execution of the executable
#   - ref_file: reference file to compare the out_file
# ~~~
# =============================================================================
execute_process(
  COMMAND ${CMAKE_COMMAND} -E env HOC_LIBRARY_PATH=${hoc_library_path} ${executable} ${exec_arg}
  WORKING_DIRECTORY ${work_dir}
  RESULT_VARIABLE status)
if(status)
  message(FATAL_ERROR "Running exec status: ${status}")
endif()
execute_process(
  COMMAND ${CMAKE_COMMAND} -E compare_files ${out_file} ${ref_file}
  WORKING_DIRECTORY ${work_dir}
  RESULT_VARIABLE status)
if(status)
  execute_process(
    COMMAND sdiff -s ${out_file} ${ref_file}
    WORKING_DIRECTORY ${work_dir}
    RESULT_VARIABLE status)
  message(FATAL_ERROR "Validating results status: ${status}")
else()
  file(REMOVE "${work_dir}/${out_file}")
endif()
