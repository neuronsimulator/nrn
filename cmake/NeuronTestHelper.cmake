# ~~~
# Utility functions for creating and managing integration tests comparing
# results obtained with NEURON and, optionally, CoreNEURON in different modes
# of operation.
#
# 1. nrn_add_test_group(NAME name
#                       SUBMODULE some/submodule
#                       MODFILE_PATTERNS mod/file/dir/*.mod other/file.mod
#                       OUTPUT datatype::file.ext otherdatatype::otherfile.ext
#                       SCRIPT_PATTERNS "*.py")
#
#    Create a new group of integration tests with the given name. The group
#    consists of different configurations of running logically the same
#    simulation. The outputs of the different configurations will be compared.
#
#    SUBMODULE        - the name of the git submodule containing test data.
#    MODFILE_PATTERNS - a list of patterns that will be matched against the
#                       submodule directory tree to find the modfiles that must
#                       be compiled (using nrnivmodl) to run the test.
#    OUTPUT           - zero or more expressions of the form `datatype::path`
#                       describing the output data produced by a test. The
#                       data type must be supported by the comparison script
#                       `compare_test_results.py`, and `path` is defined
#                       relative to the working directory in which the test
#                       command is run.
#    SCRIPT_PATTERNS  - zero or more glob expressions, defined relative to the
#                       submodule directory, matching scripts that must be
#                       copied from the submodule to the working directory in
#                       which the test is run.
#
#    The SUBMODULE, MODFILE_PATTERNS, OUTPUT and SCRIPT_PATTERNS arguments are
#    default values that will be inherited tests that are added to this group
#    using nrn_add_test. They can be overriden for specific tests by passing
#    the same keyword arguments to nrn_add_test.
#
# 2. nrn_add_test(GROUP group_name
#                 NAME test_name
#                 COMMAND command [arg ...]
#                 [REQUIRES feature1 ...]
#                 [CONFLICTS feature1 ...]
#                 [SUBMODULE some/submodule]
#                 [MODFILE_PATTERNS mod_file_pattern ...]
#                 [OUTPUT datatype::file.ext otherdatatype::otherfile.ext ...]
#                 [SCRIPT_PATTERNS "*.py" ...])
#
#    Create a new integration test inside the given group, which must have
#    previously been created using nrn_add_test_group. The COMMAND option is
#    the test expression, which is run in an environment whose PATH includes
#    the `special` binary built from the specified modfiles. The SUBMODULE,
#    MODFILE_PATTERNS, OUTPUT and SCRIPT_PATTERNS arguments are optional and
#    can be used to override the defaults defined when nrn_add_test_group is
#    called. The REQUIRES and CONFLICTS arguments allow a test to be disabled
#    if certain features are, or are not, available. Seven features are currently
#    supported: coreneuron, cpu, gpu, mod_compatibility, mpi, nmodl and python.
#
# 3. nrn_add_test_group_comparison(GROUP group_name
#                                  REFERENCE_OUTPUT datatype::file.ext [...])
#
#    Add a test job that runs after all the tests in this group (as defined by
#    prior calls to nrn_add_test) and compares their output data. The optional
#    REFERENCE_OUTPUT argument adds reference data files from the repository to
#    the comparison job with the magic name "reference_file". Paths are
#    specified relative to the root of the NEURON repository.
# ~~~
function(nrn_add_test_group)
  # NAME is used as a key, everything else is a default that can be overriden in subsequent calls to
  # nrn_add_test
  set(oneValueArgs NAME SUBMODULE)
  set(multiValueArgs OUTPUT SCRIPT_PATTERNS MODFILE_PATTERNS)
  cmake_parse_arguments(NRN_ADD_TEST_GROUP "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(DEFINED NRN_ADD_TEST_GROUP_MISSING_VALUES)
    message(
      WARNING
        "nrn_add_test: missing values for keyword arguments: ${NRN_ADD_TEST_GROUP_MISSING_VALUES}")
  endif()
  if(DEFINED NRN_ADD_TEST_GROUP_UNPARSED_ARGUMENTS)
    message(WARNING "nrn_add_test: unknown arguments: ${NRN_ADD_TEST_GROUP_UNPARSED_ARGUMENTS}")
  endif()

  # Store the default values for this test group in parent-scope variables based on the group name
  set(prefix NRN_TEST_GROUP_${NRN_ADD_TEST_GROUP_NAME})
  set(${prefix}_DEFAULT_OUTPUT
      "${NRN_ADD_TEST_GROUP_OUTPUT}"
      PARENT_SCOPE)
  set(${prefix}_DEFAULT_SUBMODULE
      "${NRN_ADD_TEST_GROUP_SUBMODULE}"
      PARENT_SCOPE)
  set(${prefix}_DEFAULT_SCRIPT_PATTERNS
      "${NRN_ADD_TEST_GROUP_SCRIPT_PATTERNS}"
      PARENT_SCOPE)
  set(${prefix}_DEFAULT_MODFILE_PATTERNS
      "${NRN_ADD_TEST_GROUP_MODFILE_PATTERNS}"
      PARENT_SCOPE)

  # Create a target that depends on all the test binaries to ensure they are actually built.
  # `nrn_add_test(...)` adds dependencies on this target.
  add_custom_target(${prefix} ALL)
endfunction()

function(nrn_add_test)
  # Parse the function arguments
  set(oneValueArgs GROUP NAME SUBMODULE PROCESSORS)
  set(multiValueArgs
      COMMAND
      OUTPUT
      SCRIPT_PATTERNS
      REQUIRES
      CONFLICTS
      MODFILE_PATTERNS
      PRECOMMAND)
  cmake_parse_arguments(NRN_ADD_TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(DEFINED NRN_ADD_TEST_MISSING_VALUES)
    message(
      WARNING "nrn_add_test: missing values for keyword arguments: ${NRN_ADD_TEST_MISSING_VALUES}")
  endif()
  if(DEFINED NRN_ADD_TEST_UNPARSED_ARGUMENTS)
    message(WARNING "nrn_add_test: unknown arguments: ${NRN_ADD_TEST_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT DEFINED NRN_TEST_ENV)
    # To avoid duplication we take this value from the {nrn}/test/CMakeLists.txt file by assuming
    # this variable name.
    message(WARNING "nrn_add_test: NRN_TEST_ENV was not defined; building test files may not work")
  endif()

  # Check if the REQUIRES and/or CONFLICTS arguments mean we should disable this test.
  set(feature_cpu_enabled ON)
  set(feature_mpi_enabled ${NRN_ENABLE_MPI})
  set(feature_python_enabled ${NRN_ENABLE_PYTHON})
  set(feature_coreneuron_enabled ${NRN_ENABLE_CORENEURON})
  if(${NRN_ENABLE_CORENEURON} OR ${NRN_ENABLE_MOD_COMPATIBILITY})
    set(feature_mod_compatibility_enabled ON)
  else()
    set(feature_mod_compatibility_enabled OFF)
  endif()
  # TODO: Do these work if CoreNEURON is installed externally?
  set(feature_gpu_enabled ${CORENRN_ENABLE_GPU})
  set(feature_nmodl_enabled ${CORENRN_ENABLE_NMODL})
  # Check REQUIRES
  set(requires_coreneuron OFF)
  foreach(required_feature ${NRN_ADD_TEST_REQUIRES})
    if(NOT DEFINED feature_${required_feature}_enabled)
      message(FATAL_ERROR "Unknown feature ${required_feature} used in REQUIRES expression")
    endif()
    if(NOT ${feature_${required_feature}_enabled})
      message(
        STATUS
          "Disabling ${NRN_ADD_TEST_GROUP}::${NRN_ADD_TEST_NAME}: ${required_feature} not enabled")
      return()
    endif()
    if(${required_feature} STREQUAL "coreneuron")
      set(requires_coreneuron ON)
    endif()
  endforeach()
  # Check CONFLICTS
  foreach(conflicting_feature ${NRN_ADD_TEST_CONFLICTS})
    if(NOT DEFINED feature_${conflicting_feature}_enabled)
      message(FATAL_ERROR "Unknown feature ${conflicting_feature} used in REQUIRES expression")
    endif()
    if(${feature_${conflicting_feature}_enabled})
      message(
        STATUS
          "Disabling ${NRN_ADD_TEST_GROUP}::${NRN_ADD_TEST_NAME}: ${conflicting_feature} enabled")
      return()
    endif()
  endforeach()

  # Get the prefix under which we stored information about this test group
  set(prefix NRN_TEST_GROUP_${NRN_ADD_TEST_GROUP})

  # Get the submodule etc. variables that have global defaults but which can be overriden locally
  set(output_files "${${prefix}_DEFAULT_OUTPUT}")
  set(git_submodule "${${prefix}_DEFAULT_SUBMODULE}")
  set(script_patterns "${${prefix}_DEFAULT_SCRIPT_PATTERNS}")
  set(modfile_patterns "${${prefix}_DEFAULT_MODFILE_PATTERNS}")
  # Override them locally if appropriate
  if(DEFINED NRN_ADD_TEST_OUTPUT)
    set(output_files "${NRN_ADD_TEST_OUTPUT}")
  endif()
  if(DEFINED NRN_ADD_TEST_SUBMODULE)
    set(git_submodule "${NRN_ADD_TEST_SUBMODULE}")
  endif()
  if(DEFINED NRN_ADD_TEST_SCRIPT_PATTERNS)
    set(script_patterns "${NRN_ADD_TEST_SCRIPT_PATTERNS}")
  endif()
  if(DEFINED NRN_ADD_TEST_MODFILE_PATTERNS)
    set(modfile_patterns "${NRN_ADD_TEST_MODFILE_PATTERNS}")
  endif()

  # First, make sure the specified submodule is initialised. If there is no submodule, everything is
  # relative to the root nrn/ directory.
  if(NOT ${git_submodule} STREQUAL "")
    initialize_submodule(external/${git_submodule})
    # Construct the name of the source tree directory where the submodule has been checked out.
    set(test_source_directory "${PROJECT_SOURCE_DIR}/external/${git_submodule}")
  else()
    set(test_source_directory "${PROJECT_SOURCE_DIR}")
  endif()
  # Construct the name of a working directory in the build tree for this group of tests
  set(group_working_directory "${PROJECT_BINARY_DIR}/test/${NRN_ADD_TEST_GROUP}")
  # Finally a working directory for this specific test within the group
  set(working_directory "${group_working_directory}/${NRN_ADD_TEST_NAME}")

  # Add a rule to build the modfiles for this test. The assumption is that it is likely that most
  # members of the group will ask for exactly the same thing, so it's worth de-duplicating. TODO:
  # allow extra arguments to be inserted here
  set(nrnivmodl_command cmake -E env ${NRN_TEST_ENV} ${CMAKE_BINARY_DIR}/bin/nrnivmodl)
  if(requires_coreneuron)
    # TODO: consider replacing the condition here with NRN_ENABLE_CORENEURON; this would tend to
    # reduce the number of times we call nrnivmodl.
    list(APPEND nrnivmodl_command -coreneuron)
  endif()
  list(APPEND nrnivmodl_command .)
  # Collect the list of modfiles that need to be compiled.
  set(modfiles)
  foreach(modfile_pattern ${modfile_patterns})
    file(GLOB pattern_modfiles "${test_source_directory}/${modfile_pattern}")
    list(APPEND modfiles ${pattern_modfiles})
  endforeach()
  if("${modfiles}" STREQUAL "")
    message(
      WARNING "Didn't find any modfiles in ${test_source_directory} using ${modfile_patterns}")
  endif()
  # Get a hash of the nrnivmodl arguments and use that to make a unique working directory
  string(SHA256 nrnivmodl_command_hash "${nrnivmodl_command};${modfiles}")
  # Construct the name of a target that refers to the compiled special binaries
  set(binary_target_name "NRN_TEST_nrnivmodl_${nrnivmodl_command_hash}")
  set(nrnivmodl_working_directory "${PROJECT_BINARY_DIR}/test/nrnivmodl/${nrnivmodl_command_hash}")
  # Short-circuit in case we set up these rules already
  if(NOT TARGET ${binary_target_name})
    # Construct the names of the modfiles in the build tree, i.e. the filenames from ${modfiles}
    # with the path ${nrnivmodl_working_directory} in front
    foreach(modfile ${modfiles})
      get_filename_component(modfile_name "${modfile}" NAME)
      list(APPEND modfile_build_paths "${nrnivmodl_working_directory}/${modfile_name}")
    endforeach()
    # Add a custom command that copies the modfiles into the build tree, the nrnivmodl command can
    # then depend on its input modfiles. copy_if_different would create the directory if we had >1
    # modfile, but in case there is only one then we should create the directory manually.
    file(MAKE_DIRECTORY "${nrnivmodl_working_directory}")
    add_custom_command(
      OUTPUT ${modfile_build_paths}
      DEPENDS ${modfiles}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${modfiles} "${nrnivmodl_working_directory}"
      COMMENT "Copying modfiles needed for test ${NRN_ADD_TEST_GROUP}::${NRN_ADD_TEST_NAME}")
    # Construct the names of the important output files
    set(special "${nrnivmodl_working_directory}/${CMAKE_HOST_SYSTEM_PROCESSOR}/special")
    # Add the custom command to generate the binaries. Get nrnivmodl from the build directory. At
    # the moment it seems that `nrnivmodl` is generated at configure time, so there is no target to
    # depend on and it should always be available, but it will try and link against libnrniv.so and
    # libcoreneuron.so so we must depend on those. TODO: could the logic of `nrnivmodl` be
    # translated to CMake, so it can be called natively here and the `nrnivmodl` executable would be
    # a wrapper that invokes CMake?
    set(output_binaries "${special}")
    set(nrnivmodl_dependencies nrniv_lib)
    if(requires_coreneuron)
      # See above; if the condition is changed to NRN_ENABLE_CORENEURON there it should be changed
      # here too.
      list(APPEND output_binaries "${special}-core")
      list(APPEND nrnivmodl_dependencies coreneuron)
      if(NOT DEFINED CORENEURON_BUILTIN_MODFILES)
        message(
          WARNING
            "nrn_add_test couldn't find the names of the builtin CoreNEURON modfiles that nrnivmodl-core implicitly depends on"
        )
      endif()
      list(APPEND nrnivmodl_dependencies ${CORENEURON_BUILTIN_MODFILES})
    endif()
    add_custom_command(
      OUTPUT ${output_binaries}
      DEPENDS ${nrnivmodl_dependencies} ${modfile_build_paths}
      COMMAND ${nrnivmodl_command}
      COMMENT "Building special[-core] for test ${NRN_ADD_TEST_GROUP}::${NRN_ADD_TEST_NAME}"
      WORKING_DIRECTORY ${nrnivmodl_working_directory})
    # Add a target that depends on the binaries
    add_custom_target(${binary_target_name} DEPENDS ${output_binaries})
    # Make the test-group-level target depend on this new target.
    add_dependencies(${prefix} ${binary_target_name})
  endif()

  # Set up the actual test. First, collect the script files that need to be copied into the test-
  # specific working directory and copy them there.
  file(MAKE_DIRECTORY "${working_directory}")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${nrnivmodl_working_directory}/${CMAKE_HOST_SYSTEM_PROCESSOR}"
            "${working_directory}/${CMAKE_HOST_SYSTEM_PROCESSOR}")
  foreach(script_pattern ${script_patterns})
    # We want to preserve directory structures, so if you pass SCRIPT_PATTERNS path/to/*.py then you
    # end up with {build_directory}/path/to/test_working_directory/path/to/script.py
    file(
      GLOB_RECURSE script_files
      RELATIVE "${test_source_directory}"
      "${test_source_directory}/${script_pattern}")
    foreach(script_file ${script_files})
      add_custom_command(
        TARGET ${prefix} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${test_source_directory}/${script_file}"
                "${working_directory}/${script_file}")
    endforeach()
  endforeach()

  # Construct the name of the test and store it in a parent-scope list to be used when setting up
  # the comparison job
  set(test_name "${NRN_ADD_TEST_GROUP}::${NRN_ADD_TEST_NAME}")
  set(group_members "${${prefix}_TESTS}")
  list(APPEND group_members "${test_name}")
  set(${prefix}_TESTS
      "${group_members}"
      PARENT_SCOPE)
  # Add the actual test job, including the `special` and `special-core` binaries in the path. TODOs:
  #
  # * Do we need to manipulate PYTHONPATH more to make `python options.py` invocations work?
  # * Using CORENEURONLIB here introduces some differences between the tests and the standard way
  #   that users run nrnivmodl and special. Ideally we would reduce such differences, without
  #   increasing build time too much (by running nrnivmodl multiple times) or compromising our
  #   ability to execute the tests in parallel (which precludes blindly running everything in the
  #   same directory).
  add_test(
    NAME "${test_name}"
    COMMAND ${CMAKE_COMMAND} -E env ${NRN_ADD_TEST_COMMAND}
    WORKING_DIRECTORY "${working_directory}")
  set(test_names ${test_name})
  if(DEFINED NRN_ADD_TEST_PRECOMMAND)
    add_test(
      NAME ${test_name}::preparation
      COMMAND ${CMAKE_COMMAND} -E env ${NRN_ADD_TEST_PRECOMMAND}
      WORKING_DIRECTORY "${working_directory}")
    list(APPEND test_names ${test_name}::preparation)
    set_tests_properties(${test_name} PROPERTIES DEPENDS ${test_name}::preparation)
  endif()
  if(DEFINED NRN_ADD_TEST_PROCESSORS)
    set(tests_properties ${test_names} PROPERTIES PROCESSORS ${NRN_ADD_TEST_PROCESSORS})
  endif()
  set_tests_properties(
    ${test_names}
    PROPERTIES
      ENVIRONMENT
      "${NRN_TEST_ENV};PATH=${nrnivmodl_working_directory}/${CMAKE_HOST_SYSTEM_PROCESSOR}:$ENV{PATH};CORENEURONLIB=${nrnivmodl_working_directory}/${CMAKE_HOST_SYSTEM_PROCESSOR}/libcorenrnmech${CMAKE_SHARED_LIBRARY_SUFFIX}"
  )

  # Construct an expression containing the names of the test output files that will be passed to the
  # comparison script.
  set(output_file_string "${NRN_ADD_TEST_NAME}")
  foreach(output_file ${output_files})
    # output_file is `type1::fname1` output_full_path is `type1::${working_directory}/fname1`
    string(REGEX REPLACE "^([^:]+)::(.*)$" "\\1::${working_directory}/\\2" output_full_path
                         "${output_file}")
    set(output_file_string "${output_file_string}::${output_full_path}")
  endforeach()
  # Add the outputs from this test to a parent-scope list that will be read by the
  # nrn_add_test_group_comparison function and used to set up a test job that compares the various
  # test results. The list has the format:
  # ~~~
  # [testname1::test1type1::test1path1::test1type2::test1path2,
  #  testname2::test2type1::test2path1::test2type2::test2path2,
  #  ...]
  # ~~~
  set(test_outputs "${${prefix}_TEST_OUTPUTS}")
  list(APPEND test_outputs "${output_file_string}")
  set(${prefix}_TEST_OUTPUTS
      "${test_outputs}"
      PARENT_SCOPE)
endfunction()

function(nrn_add_test_group_comparison)
  # Parse function arguments
  set(oneValueArgs GROUP)
  set(multiValueArgs REFERENCE_OUTPUT)
  cmake_parse_arguments(NRN_ADD_TEST_GROUP_COMPARISON "" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})
  if(NOT DEFINED NRN_ADD_TEST_GROUP_COMPARISON_GROUP)
    message(
      ERROR
      "nrn_add_test_group_comparison was not told which test group to compare results from. Please pass a GROUP keyword argument."
    )
  endif()
  if(DEFINED NRN_ADD_TEST_GROUP_COMPARISON_MISSING_VALUES)
    message(
      WARNING
        "nrn_add_test: missing values for keyword arguments: ${NRN_ADD_TEST_GROUP_COMPARISON_MISSING_VALUES}"
    )
  endif()
  if(DEFINED NRN_ADD_TEST_GROUP_COMPARISON_UNPARSED_ARGUMENTS)
    message(
      WARNING "nrn_add_test: unknown arguments: ${NRN_ADD_TEST_GROUP_COMPARISON_UNPARSED_ARGUMENTS}"
    )
  endif()

  # Get the prefix under which we stored information about this test group
  set(prefix NRN_TEST_GROUP_${NRN_ADD_TEST_GROUP_COMPARISON_GROUP})
  # Check if there are any tests to compare the results from. It might be that the REQUIRES and
  # CONFLICTS clauses disable all the tests in a group.
  set(test_list "${${prefix}_TESTS}") # indirection via the test_list variable is needed for old
                                      # CMake
  list(LENGTH test_list num_tests)
  if(NOT ${num_tests})
    message(
      STATUS
        "Skipping comparison job for group ${NRN_ADD_TEST_GROUP_COMPARISON_GROUP} because no tests were enabled."
    )
    return()
  endif()

  # If REFERENCE_OUTPUT is passed it is a list of entries of the form
  # datatype::path_relative_to_nrn. Different test jobs are in principle allowed to come from
  # different repositories, so we don't insert the repository name automagically. Construct the
  # working directory for the comparison job.
  set(test_directory "${PROJECT_BINARY_DIR}/test")
  # Get the list of reference files and copy them into the build directory. Starting from
  # ~~~
  #   type1::path1;type2::path2
  # ~~~
  # assemble
  # ~~~
  #   reference_file::type1::{test_directory}/path1::type2::{test_directory}/path2
  # ~~~
  # i.e. make sure the expression passed to the comparison script refers to the copies of the
  # reference files in the build directory, and that there is a title string ("reference_file") for
  # the comparison script to use in its reports.
  set(reference_file_string "reference_file")
  foreach(reference_expression ${NRN_ADD_TEST_GROUP_COMPARISON_REFERENCE_OUTPUT})
    # reference_expression is datatype::reference_path, extract `reference_path`
    string(REGEX REPLACE "^[^:]+::(.*)$" "\\1" reference_path "${reference_expression}")
    # construct `datatype::{test_directory}/reference_path`
    string(REGEX REPLACE "^([^:]+)::(.*)$" "\\1::${test_directory}/\\2"
                         reference_file_string_addition "${reference_expression}")
    set(reference_file_string "${reference_file_string}::${reference_file_string_addition}")
    add_custom_command(
      TARGET ${prefix} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/${reference_path}"
              "${test_directory}/${reference_path}"
      COMMENT "Copying reference file for test group ${NRN_ADD_TEST_GROUP_COMPARISON_GROUP}")
  endforeach()

  # Copy the comparison script
  add_custom_command(
    TARGET ${prefix} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/test/scripts/compare_test_results.py" "${test_directory}"
    COMMENT "Copying test comparison script for test group ${NRN_ADD_TEST_GROUP_COMPARISON_GROUP}")

  # Add a test job that compares the results of the previous test jobs
  set(comparison_name "${NRN_ADD_TEST_GROUP_COMPARISON_GROUP}::compare_results")
  add_test(
    NAME ${comparison_name}
    COMMAND "${test_directory}/compare_test_results.py" ${${prefix}_TEST_OUTPUTS}
            ${reference_file_string}
    WORKING_DIRECTORY "${test_directory}/${NRN_ADD_TEST_GROUP_COMPARISON_GROUP}")

  # Make sure the comparison job declares that it depends on the previous jobs. The comparison job
  # will always run, but the dependencies ensure that it will be sequenced correctly, i.e. it runs
  # after the jobs it is comparing.
  set_tests_properties(${comparison_name} PROPERTIES DEPENDS "${${prefix}_TESTS}")
  # Set up the environment for the test comparison job
  set_tests_properties(${comparison_name} PROPERTIES ENVIRONMENT
                                                     "PATH=${CMAKE_BINARY_DIR}/bin:$ENV{PATH}")
endfunction()
