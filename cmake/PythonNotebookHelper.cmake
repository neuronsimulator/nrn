# execute a single notebook from `INPUT_PATH` inplace
function(add_notebook target)
  cmake_parse_arguments(opt "" "INPUT_PATH;WORKING_DIRECTORY" "ENV" ${ARGN})

  # name of the file
  get_filename_component(filename "${opt_INPUT_PATH}" NAME)

  add_custom_target(
    "${target}"
    COMMAND ${CMAKE_COMMAND} -E env ${opt_ENV} jupyter nbconvert --to notebook --execute --inplace
            "${opt_INPUT_PATH}"
    WORKING_DIRECTORY "${opt_WORKING_DIRECTORY}"
    VERBATIM)

  # the one used for cleanup
  add_custom_target(
    "clean_${target}"
    COMMAND
      ${CMAKE_COMMAND} -E env ${opt_ENV} jupyter nbconvert --ClearOutputPreprocessor.enabled=True
      --ClearMetadataPreprocessor.enabled=True --clear-output --inplace "${opt_INPUT_PATH}")
endfunction()
