execute_process(COMMAND ${executable} ${exec_arg}
                WORKING_DIRECTORY ${work_dir}
                RESULT_VARIABLE status)
if(status)
  message(FATAL_ERROR "Running exec status: ${status}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${out_file} ${ref_file}
                WORKING_DIRECTORY ${work_dir}
                RESULT_VARIABLE status)
if(status)
  message(FATAL_ERROR "Validating results status: ${status}")
else()
  file(REMOVE "${work_dir}/${out_file}")
endif()
