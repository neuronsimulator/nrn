# Execute nrniv on a HOC test and optionally compare output to a reference file.
#
# Arguments: - executable: nrniv executable path - test_path: path to the test directory; must
# contain <basename>.hoc if <basename>.out.ref exists, the produced <basename>.out (in the working
# directory) is compared against it - work_dir: working directory where output files are produced

# Derive the test name from the directory basename
get_filename_component(test_name "${test_path}" NAME)

set(hoc_file "${test_path}/${test_name}.hoc")
if(NOT EXISTS "${hoc_file}")
  message(FATAL_ERROR "HOC file not found: ${hoc_file}")
endif()

file(MAKE_DIRECTORY "${work_dir}")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E env HOC_LIBRARY_PATH=${test_path} ${executable} -nobanner -c
          "strdef BASE" -c "BASE = \"${test_path}\"" ${hoc_file}
  WORKING_DIRECTORY ${work_dir}
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
  RESULT_VARIABLE status)
if(status)
  message(FATAL_ERROR "nrniv failed with status: ${status}: `${stdout}` / `${stderr}`")
endif()

# Compare output to reference if a .out.ref file exists
set(ref_file "${test_path}/${test_name}.out.ref")
if(EXISTS "${ref_file}")
  set(out_file "${work_dir}/${test_name}.out")
  if(NOT EXISTS "${out_file}")
    message(FATAL_ERROR "Expected output file not produced: ${out_file}")
  endif()

  execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${out_file} ${ref_file}
                  RESULT_VARIABLE status)

  if(status)
    execute_process(COMMAND sdiff -s ${out_file} ${ref_file})
    message(FATAL_ERROR "Output differs from reference: `sdiff -s ${out_file} ${ref_file}`")
  else()
    file(REMOVE "${out_file}")
  endif()
endif()
