# ~~~
# Utility functions for creating and managing integration tests comparing
# results obtained with NEURON and, optionally, CoreNEURON in different modes
# of operation.
#
# 1. nrn_add_test_group(NAME name
#                       SUBMODULE some/submodule
#                       MODFILE_DIRECTORY mod_file_dir
#                       OUTPUT datatype::file.ext otherdatatype::otherfile.ext
#                       SCRIPT_PATTERNS "*.py")
#
#    Create a new group of integration tests with the given name. The group
#    consists of different configurations of running logically the same
#    simulation. The outputs of the different configurations will be compared.
#
#    SUBMODULE         - the name of the git submodule containing test data.
#    MODFILE_DIRECTORY - a relative path inside the submodule that contains the
#                        modfiles that must be compiled (using nrnivmodl) to
#                        run the test.
#    OUTPUT            - zero or more expressions of the form `datatype::path`
#                        describing the output data produced by a test. The
#                        data type must be supported by the comparison script
#                        `compare_test_results.py`, and `path` is defined
#                        relative to the working directory in which the test
#                        command is run.
#    SCRIPT_PATTERNS   - zero or more glob expressions, defined relative to the
#                        submodule directory, matching scripts that must be
#                        copied from the submodule to the working directory in
#                        which the test is run.
#
#    The SUBMODULE, MODFILE_DIRECTORY, OUTPUT and SCRIPT_PATTERNS arguments are
#    default values that will be inherited tests that are added to this group
#    using nrn_add_test. They can be overriden for specific tests by passing
#    the same keyword arguments to nrn_add_test.
#
# 2. nrn_add_test(GROUP group_name
#                 NAME test_name
#                 [REQUIRES feature1 ...]
#                 [CONFLICTS feature1 ...]
#                 [SUBMODULE some/submodule]
#                 [MODFILE_DIRECTORY mod_file_dir]
#                 [OUTPUT datatype::file.ext otherdatatype::otherfile.ext ...]
#                 [SCRIPT_PATTERNS "*.py" ...])
#
#    Create a new integration test inside the given group, which must have
#    previously been created using nrn_add_test_group. The SUBMODULE,
#    MODFILE_DIRECTORY, OUTPUT and SCRIPT_PATTERNS arguments are optional and
#    can be used to override the defaults defined when nrn_add_test_group is
#    called. The REQUIRES and CONFLICTS arguments allow a test to be disabled
#    if certain features are, or are not, available. Four features are
#    currently supported: coreneuron, gpu, nmodl and mod_compatibility.
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
  set(oneValueArgs NAME SUBMODULE MODFILE_DIRECTORY)
  set(multiValueArgs OUTPUT SCRIPT_PATTERNS)
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
  set(${prefix}_DEFAULT_MODFILE_DIRECTORY
      "${NRN_ADD_TEST_GROUP_MODFILE_DIRECTORY}"
      PARENT_SCOPE)
  # Create a target that depends on all the test binaries to ensure they are actually built.
  # `nrn_add_test(...)` adds dependencies on this target.
  add_custom_target(${prefix} ALL)
endfunction()

function(nrn_add_test)
  set(oneValueArgs GROUP NAME SUBMODULE MODFILE_DIRECTORY)
  set(multiValueArgs COMMAND OUTPUT SCRIPT_PATTERNS REQUIRES CONFLICTS)
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
  set(feature_coreneuron_enabled ${NRN_ENABLE_CORENEURON})
  if(${NRN_ENABLE_CORENEURON} OR ${NRN_ENABLE_MOD_COMPATIBILITY})
    set(feature_mod_compatibility_enabled ON)
  else()
    set(feature_mod_compatibility_enabled OFF)
  endif()
  # TODO: Does these work if CoreNEURON is installed externally?
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
  set(modfile_directory "${${prefix}_DEFAULT_MODFILE_DIRECTORY}")
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
  if(DEFINED NRN_ADD_TEST_MODFILE_DIRECTORY)
    set(modfile_directory "${NRN_ADD_TEST_MODFILE_DIRECTORY}")
  endif()
  # First, make sure the specified submodule is initialised.
  initialize_submodule(external/${git_submodule})
  # Construct the name of the source tree directory where the submodule has been checked out.
  set(test_source_directory "${PROJECT_SOURCE_DIR}/external/${git_submodule}")
  # Construct the name of a working directory in the build tree for this group of tests
  set(group_working_directory "${PROJECT_BINARY_DIR}/test/external/${NRN_ADD_TEST_GROUP}")
  # Finally a working directory for this specific test within the group
  set(working_directory "${group_working_directory}/${NRN_ADD_TEST_NAME}")
  # Add a rule to build the modfiles for this test. The assumption is that it is likely that most
  # members of the group will ask for exactly the same thing, so it's worth de-duplicating. TODO:
  # allow extra arguments to be inserted here
  set(nrnivmodl_command cmake -E env ${NRN_TEST_ENV} $ENV{SHELL} nrnivmodl)
  if(requires_coreneuron)
    list(APPEND nrnivmodl_command -coreneuron)
  endif()
  list(APPEND nrnivmodl_command .)
  # Collect the list of modfiles that need to be compiled.
  file(GLOB modfiles "${test_source_directory}/${modfile_directory}/*.mod")
  # Get a hash of the nrnivmodl arguments and use that to make a unique working directory
  string(SHA256 nrnivmodl_command_hash "${nrnivmodl_command};${modfiles}")
  # Construct the name of a target that refers to the compiled special binaries
  set(binary_target_name "${prefix}_${nrnivmodl_command_hash}")
  set(nrnivmodl_working_directory "${group_working_directory}/${nrnivmodl_command_hash}")
  # Short-circuit in case we set up these rules already
  if(NOT TARGET ${binary_target_name})
    # Construct the names of the important output files
    set(special "${nrnivmodl_working_directory}/${CMAKE_HOST_SYSTEM_PROCESSOR}/special")
    # Add the custom command to generate the binaries. Get nrnivmodl from the build directory. At
    # the moment it seems that `nrnivmodl` is generated a configure time, so there is no target to
    # depend on and it should always be available, but it will try and link against libnrniv.so and
    # libcoreneuron.so so we must depend on those. TODO: could the logic of `nrnivmodl` be
    # translated to CMake, so it can be called natively here and the `nrnivmodl` executable would be
    # a wrapper that invokes CMake?
    set(output_binaries "${special}")
    set(nrnivmodl_dependencies nrniv_lib)
    if(requires_coreneuron)
      list(APPEND output_binaries "${special}-core")
      list(APPEND nrnivmodl_dependencies coreneuron)
    endif()
    add_custom_command(
      OUTPUT ${output_binaries}
      DEPENDS ${nrnivmodl_dependencies}
      COMMAND ${CMAKE_COMMAND} -E env ${NRN_TEST_ENV} PATH=${CMAKE_BINARY_DIR}/bin:$ENV{PATH}
              ${nrnivmodl_command}
      WORKING_DIRECTORY ${nrnivmodl_working_directory})
    # Add a target that depends on the binaries
    add_custom_target(${binary_target_name} DEPENDS ${output_binaries})
    # Make sure the modfiles are copied to the working directory before nrnivmodl is run
    file(MAKE_DIRECTORY "${nrnivmodl_working_directory}")
    add_custom_command(
      TARGET ${binary_target_name} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                                     ${modfiles} "${nrnivmodl_working_directory}")
    # Make the test-group-level target depend on this new target.
    add_dependencies(${prefix} ${binary_target_name})
  endif()
  # Set up the actual test. First, collect the script files that need to be copied into the test-
  # specific working directory and copy them there.
  list(TRANSFORM script_patterns PREPEND "${test_source_directory}/")
  file(GLOB script_files ${script_patterns})
  file(MAKE_DIRECTORY "${working_directory}")
  add_custom_command(TARGET ${prefix} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                                         ${script_files} "${working_directory}")
  # Construct the name of the test and store it in a parent-scope list to be used when setting up
  # the comparison job
  set(test_name "${NRN_ADD_TEST_GROUP}::${NRN_ADD_TEST_NAME}")
  set(group_members "${${prefix}_TESTS}")
  list(APPEND group_members "${test_name}")
  set(${prefix}_TESTS
      "${group_members}"
      PARENT_SCOPE)
  # Add the actual test job, including the `special` and `special-core` binaries in the path. TODO:
  # do we need to manipulate PYTHONPATH too to make `python options.py` invocations work?
  add_test(
    NAME "${test_name}"
    COMMAND ${CMAKE_COMMAND} -E env ${NRN_TEST_ENV}
            PATH=${nrnivmodl_working_directory}/${CMAKE_HOST_SYSTEM_PROCESSOR}:$ENV{PATH}
            ${NRN_ADD_TEST_COMMAND}
    WORKING_DIRECTORY "${working_directory}")
  # Construct the names of the test output files. ${output_files} is a list of form
  # type1::fname1;type2::fname2;... we would like to transform it to
  # type1::${working_directory}/fname1;type2::${working_directory}/fname2;...
  list(TRANSFORM output_files REPLACE "^([^:]+)::(.*)$" "\\1::${working_directory}/\\2")
  list(JOIN output_files "::" output_file_string)
  # Add the outputs from this test to a parent-scope list that will be read by the
  # nrn_add_test_group_comparison function and used to set up a test job that compares the various
  # test results. The list has the format:
  # [testname1::test1type1::test1path1::test1type2::test1path2,
  # testname2::test2type1::test2path1::test2type2::test2path2, ...]
  set(test_outputs "${${prefix}_TEST_OUTPUTS}")
  list(APPEND test_outputs "${NRN_ADD_TEST_NAME}::${output_file_string}")
  set(${prefix}_TEST_OUTPUTS
      "${test_outputs}"
      PARENT_SCOPE)
endfunction()

function(nrn_add_test_group_comparison)
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
  list(LENGTH "${${prefix}_TESTS}" num_tests)
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
  set(external_test_directory "${PROJECT_BINARY_DIR}/test/external")
  set(working_directory "${external_test_directory}/${NRN_ADD_TEST_GROUP_COMPARISON_GROUP}")
  # Get the list of reference files and copy them into the build directory
  foreach(reference_expression ${NRN_ADD_TEST_GROUP_COMPARISON_REFERENCE_OUTPUT})
    # reference_expression is datatype::reference_path, extract reference_path
    string(REGEX REPLACE "^[^:]+::(.*)$" "\\1" reference_path "${reference_expression}")
    add_custom_command(
      TARGET ${prefix} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/${reference_path}"
              "${external_test_directory}/${reference_path}")
  endforeach()
  # Transform the list of reference expressions to include ${external_test_directory} (i.e. use the
  # copied versions of the reference files) and insert the title "reference_file" because the
  # comparison script requires that all the data files it receives have a named source.
  list(TRANSFORM NRN_ADD_TEST_GROUP_COMPARISON_REFERENCE_OUTPUT REPLACE "^([^:]+)::(.*)$"
       "reference_file::\\1::${external_test_directory}/\\2")
  # Copy the comparison script
  add_custom_command(
    TARGET ${prefix} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/test/scripts/compare_test_results.py"
            "${external_test_directory}")
  # Add a test job that compares the results of the previous test jobs
  add_test(
    NAME "${NRN_ADD_TEST_GROUP_COMPARISON_GROUP}::compare_results"
    COMMAND
      ${CMAKE_COMMAND} -E env PATH=${CMAKE_BINARY_DIR}/bin:$ENV{PATH}
      "${external_test_directory}/compare_test_results.py" ${${prefix}_TEST_OUTPUTS}
      ${NRN_ADD_TEST_GROUP_COMPARISON_REFERENCE_OUTPUT}
    WORKING_DIRECTORY "${working_directory}")
  # Make sure the comparison job declares that it depends on the previous jobs
  set_tests_properties("${NRN_ADD_TEST_GROUP_COMPARISON_GROUP}::compare_results"
                       PROPERTIES DEPENDS "${${prefix}_TESTS}")
endfunction()
