# ~~~
# Utility functions for creating and managing integration tests comparing
# results obtained with NEURON and, optionally, CoreNEURON in different modes
# of operation.
#
# 1. nrn_add_test_group(NAME name
#                       [CORENEURON]
#                       [ENVIRONMENT var1=val2 ...]
#                       [MODFILE_PATTERNS mod/file/dir/*.mod ...]
#                       [NRNIVMODL_ARGS arg1 ...]
#                       [OUTPUT datatype::file.ext ...]
#                       [SCRIPT_PATTERNS "*.py" ...]
#                       [SIM_DIRECTORY sim_dir]
#                       [SUBMODULE some/submodule]
#                      )
#
#    Create a new group of integration tests with the given name. The group
#    consists of different configurations of running logically the same
#    simulation. The outputs of the different configurations will be compared.
#
#    CORENEURON       - if this is passed, the mechanisms will be compiled for
#                       CoreNEURON if CoreNEURON is enabled. You should pass
#                       this if any test in the group is going to include
#                       `coreneuron` in its REQUIRES clause.
#    ENVIRONMENT      - extra environment variables that should be set when the
#                       test is run. These must not overlap with the variables
#                       that are automatically set internally, such as PATH.
#    MODFILE_PATTERNS - a list of patterns that will be matched against the
#                       submodule directory tree to find the modfiles that must
#                       be compiled (using nrnivmodl) to run the test. The special
#                       value "NONE" will not emit a warning if nothing matches it
#                       and will stop nrnivmodl from being called.
#    NRNIVMODL_ARGS   - extra arguments that will be passed to nrnivmodl.
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
#    SIM_DIRECTORY    - extra directory name under which the test will be run,
#                       this is useful for some models whose scripts make
#                       assumptions about directory layout.
#    SUBMODULE        - the name of the git submodule containing test data.
#
#    The ENVIRONMENT, OUTPUT, SCRIPT_PATTERNS and SIM_DIRECTORY arguments are
#    default values that will be inherited by tests that are added to this
#    group using nrn_add_test. They can be overriden for specific tests by
#    passing the same keyword arguments to nrn_add_test.
#
# 2. nrn_add_test(GROUP group_name
#                 NAME test_name
#                 COMMAND command [arg ...]
#                 [ENVIRONMENT VAR1=value1 ...]
#                 [PRELOAD_SANITIZER]
#                 [CONFLICTS feature1 ...]
#                 [PRECOMMAND command ...]
#                 [PROCESSORS required_processors]
#                 [REQUIRES feature1 ...]
#                 [OUTPUT datatype::file.ext ...]
#                 [SCRIPT_PATTERNS "*.py" ...]
#                 [SIM_DIRECTORY sim_dir]
#                )
#
#    Create a new integration test inside the given group, which must have
#    previously been created using nrn_add_test_group. The COMMAND option is
#    the test expression, which is run in an environment whose PATH includes
#    the `special` binary built from the specified modfiles.
#    The REQUIRES and CONFLICTS arguments allow a test to be disabled if
#    certain features are, or are not, available. Eight features are currently
#    supported: coreneuron, cpu, gpu, mod_compatibility, mpi, mpi_dynamic,
#    nmodl and python. The PRECOMMAND argument is an optional command that will
#    be executed before COMMAND and in the same directory. It can be used to
#    prepare input data for simulations. The PROCESSORS argument specifies the
#    number of processors used by the test. This is passed to CTest and allows
#    invocations such as `ctest -j 16` to avoid overcommitting resources by
#    running too many tests with internal parallelism. The PRELOAD_SANITIZER
#    flag controls whether or not the PRELOAD flag is passed to
#    cpp_cc_configure_sanitizers; this needs to be set when the test executable
#    is *not* built by NEURON, typically because it is `python`.
#    The remaining arguments can documented in nrn_add_test_group. The default
#    values specified there can be overriden on a test-by-test basis by passing
#    the same arguments here.
#
# 3. nrn_add_test_group_comparison(GROUP group_name
#                                  [REFERENCE_OUTPUT datatype::file.ext ...])
#
#    Add a test job that runs after all the tests in this group (as defined by
#    prior calls to nrn_add_test) and compares their output data. The optional
#    REFERENCE_OUTPUT argument adds reference data files from the repository to
#    the comparison job with the magic name "reference_file". Paths are
#    specified relative to the root of the NEURON repository.
# ~~~
# Load the cpp_cc_build_time_copy helper function.
include("${CODING_CONV_CMAKE}/build-time-copy.cmake")
function(nrn_add_test_group)
  # NAME is used as a key, [CORENEURON, MODFILE_PATTERNS, NRNIVMODL_ARGS and SUBMODULE] are used to
  # set up a custom target that runs nrnivmod, everything else is a default that can be overriden in
  # subsequent calls to nrn_add_test that actually set up CTest tests.
  set(options CORENEURON)
  set(oneValueArgs NAME SUBMODULE)
  set(multiValueArgs ENVIRONMENT MODFILE_PATTERNS NRNIVMODL_ARGS OUTPUT SCRIPT_PATTERNS
                     SIM_DIRECTORY)
  cmake_parse_arguments(NRN_ADD_TEST_GROUP "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})
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
  set(${prefix}_DEFAULT_ENVIRONMENT
      "${NRN_ADD_TEST_GROUP_ENVIRONMENT}"
      PARENT_SCOPE)
  set(${prefix}_DEFAULT_OUTPUT
      "${NRN_ADD_TEST_GROUP_OUTPUT}"
      PARENT_SCOPE)
  set(${prefix}_DEFAULT_SCRIPT_PATTERNS
      "${NRN_ADD_TEST_GROUP_SCRIPT_PATTERNS}"
      PARENT_SCOPE)
  set(${prefix}_DEFAULT_SIM_DIRECTORY
      "${NRN_ADD_TEST_GROUP_SIM_DIRECTORY}"
      PARENT_SCOPE)
  # Set up a target that runs nrnivmodl for this group of tests. First, make sure the specified
  # submodule is initialised. If there is no submodule, everything is relative to the root nrn/
  # directory.
  if(NOT ${NRN_ADD_TEST_GROUP_SUBMODULE} STREQUAL "")
    cpp_cc_git_submodule(${NRN_ADD_TEST_GROUP_SUBMODULE} QUIET)
    # Construct the name of the source tree directory where the submodule has been checked out.
    set(test_source_directory "${PROJECT_SOURCE_DIR}/external/${NRN_ADD_TEST_GROUP_SUBMODULE}")
  else()
    set(test_source_directory "${PROJECT_SOURCE_DIR}")
  endif()
  set(${prefix}_TEST_SOURCE_DIRECTORY
      "${test_source_directory}"
      PARENT_SCOPE)
  if(NOT DEFINED NRN_RUN_FROM_BUILD_DIR_ENV)
    # To avoid duplication we take this value from the {nrn}/test/CMakeLists.txt file by assuming
    # this variable name.
    message(WARNING "nrn_add_test: NRN_RUN_FROM_BUILD_DIR_ENV was not defined;"
                    " building test files may not work")
  endif()
  if(NOT "${NRN_ADD_TEST_GROUP_MODFILE_PATTERNS}" STREQUAL "NONE")
    # Add a rule to build the modfiles for this test group. Multiple groups may ask for exactly the
    # same thing (testcorenrn), so it's worth deduplicating.
    set(hash_components ${NRN_ADD_TEST_GROUP_NRNIVMODL_ARGS})
    # Escape special characters (problematic with Windows paths when calling nrnivmodl)
    string(REGEX REPLACE "([][+.*()^])" "\\\\\\1" NRN_RUN_FROM_BUILD_DIR_ENV
                         "${NRN_RUN_FROM_BUILD_DIR_ENV}")
    set(nrnivmodl_command cmake -E env ${NRN_RUN_FROM_BUILD_DIR_ENV}
                          ${CMAKE_BINARY_DIR}/bin/nrnivmodl ${NRN_ADD_TEST_GROUP_NRNIVMODL_ARGS})
    # The user decides whether or not this test group should have its MOD files compiled for
    # CoreNEURON.
    set(nrnivmodl_dependencies)
    if(NRN_ADD_TEST_GROUP_CORENEURON AND NRN_ENABLE_CORENEURON)
      list(APPEND hash_components -coreneuron)
      list(APPEND nrnivmodl_dependencies ${CORENEURON_TARGET_TO_DEPEND})
      list(APPEND nrnivmodl_command -coreneuron)
    endif()
    list(APPEND nrnivmodl_command .)
    # Collect the list of modfiles that need to be compiled.
    set(modfiles)
    foreach(modfile_pattern ${NRN_ADD_TEST_GROUP_MODFILE_PATTERNS})
      file(GLOB pattern_modfiles
           "${test_source_directory}/${NRN_ADD_TEST_GROUP_SIM_DIRECTORY}/${modfile_pattern}")
      list(APPEND modfiles ${pattern_modfiles})
    endforeach()
    if("${modfiles}" STREQUAL "")
      message(WARNING "Didn't find any modfiles in ${test_source_directory} "
                      "using ${NRN_ADD_TEST_GROUP_MODFILE_PATTERNS}")
    endif()
    list(SORT modfiles)
    foreach(modfile ${modfiles})
      # ${modfile} is an absolute path starting with ${PROJECT_SOURCE_DIR}, let's only add the part
      # below this common prefix to the hash
      string(LENGTH "${PROJECT_SOURCE_DIR}/" prefix_length)
      string(SUBSTRING "${modfile}" ${prefix_length} -1 relative_modfile)
      list(APPEND hash_components "${relative_modfile}")
    endforeach()
    # Get a hash that forms the working directory for nrnivmodl.
    string(SHA256 nrnivmodl_command_hash "${hash_components}")
    # Construct the name of a target that refers to the compiled special binaries
    set(binary_target_name "NRN_TEST_nrnivmodl_${nrnivmodl_command_hash}")
    set(nrnivmodl_directory "${PROJECT_BINARY_DIR}/test/nrnivmodl/${nrnivmodl_command_hash}")
    # Short-circuit if the target has already been created.
    if(NOT TARGET "${binary_target_name}")
      # Copy modfiles from source -> build tree.
      foreach(modfile ${modfiles})
        # Construct the build tree path of the modfile.
        get_filename_component(modfile_name "${modfile}" NAME)
        set(modfile_build_path "${nrnivmodl_directory}/${modfile_name}")
        # Add a build rule that copies this modfile from the source tree to the build tree.
        cpp_cc_build_time_copy(
          INPUT "${modfile}"
          OUTPUT "${modfile_build_path}"
          NO_TARGET)
        # Store a list of the modfile paths in the build tree so we can declare nrnivmodl's
        # dependency on these.
        list(APPEND modfile_build_paths "${modfile_build_path}")
      endforeach()
      # Construct the names of the important output files
      set(special "${nrnivmodl_directory}/${CMAKE_HOST_SYSTEM_PROCESSOR}/special")
      # Add the custom command to generate the binaries. Get nrnivmodl from the build directory. At
      # the moment it seems that `nrnivmodl` is generated at configure time, so there is no target
      # to depend on and it should always be available, but it will try and link against libnrniv.so
      # and libcorenrnmech.so so we must depend on those. TODO: could the logic of `nrnivmodl` be
      # translated to CMake, so it can be called natively here and the `nrnivmodl` executable would
      # be a wrapper that invokes CMake?
      set(output_binaries "${special}")
      list(APPEND nrnivmodl_dependencies nrniv_lib)
      if(NRN_ENABLE_CORENEURON AND NRN_ADD_TEST_GROUP_CORENEURON)
        list(APPEND output_binaries "${special}-core")
        if((NOT coreneuron_FOUND) AND (NOT DEFINED CORENEURON_BUILTIN_MODFILES))
          message(WARNING "nrn_add_test_group couldn't find the names of the builtin "
                          "CoreNEURON modfiles that nrnivmodl-core implicitly depends "
                          "on *and* CoreNEURON is being built internally")
        endif()
        list(APPEND nrnivmodl_dependencies ${CORENEURON_BUILTIN_MODFILES})
      endif()
      add_custom_command(
        OUTPUT ${output_binaries}
        DEPENDS ${nrnivmodl_dependencies} ${modfile_build_paths}
        COMMAND ${nrnivmodl_command}
        COMMENT "Building special[-core] for test group ${NRN_ADD_TEST_GROUP_NAME}"
        WORKING_DIRECTORY "${nrnivmodl_directory}")
      # Add a target that depends on the binaries that will always be built.
      add_custom_target(${binary_target_name} DEPENDS ${output_binaries})
    endif()
    set(${prefix}_NRNIVMODL_DIRECTORY
        "${nrnivmodl_directory}"
        PARENT_SCOPE)
    set(${prefix}_NRNIVMODL_TARGET_NAME
        "${binary_target_name}"
        PARENT_SCOPE)
  endif()
endfunction()

function(nrn_add_test)
  # Parse the function arguments
  set(oneValueArgs GROUP NAME PROCESSORS)
  set(multiValueArgs
      COMMAND
      ENVIRONMENT
      CONFLICTS
      PRECOMMAND
      REQUIRES
      OUTPUT
      SCRIPT_PATTERNS
      SIM_DIRECTORY)
  cmake_parse_arguments(NRN_ADD_TEST "PRELOAD_SANITIZER" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})
  if(DEFINED NRN_ADD_TEST_MISSING_VALUES)
    message(
      WARNING "nrn_add_test: missing values for keyword arguments: ${NRN_ADD_TEST_MISSING_VALUES}")
  endif()
  if(DEFINED NRN_ADD_TEST_UNPARSED_ARGUMENTS)
    message(WARNING "nrn_add_test: unknown arguments: ${NRN_ADD_TEST_UNPARSED_ARGUMENTS}")
  endif()
  # Check if the REQUIRES and/or CONFLICTS arguments mean we should disable this test.
  set(feature_cpu_enabled ON)
  set(feature_mpi_enabled ${NRN_ENABLE_MPI})
  set(feature_mpi_dynamic_enabled ${NRN_ENABLE_MPI_DYNAMIC})
  set(feature_python_enabled ${NRN_ENABLE_PYTHON})
  set(feature_coreneuron_enabled ${NRN_ENABLE_CORENEURON})
  if(NRN_ENABLE_CORENEURON AND CORENRN_ENABLE_SHARED)
    set(feature_coreneuron_shared_enabled ON)
  else()
    set(feature_coreneuron_shared_enabled OFF)
  endif()
  if(NRN_ENABLE_CORENEURON OR NRN_ENABLE_MOD_COMPATIBILITY)
    set(feature_mod_compatibility_enabled ON)
  else()
    set(feature_mod_compatibility_enabled OFF)
  endif()
  set(feature_gpu_enabled ${CORENRN_ENABLE_GPU})
  set(feature_nmodl_enabled ${CORENRN_ENABLE_NMODL})
  # Check REQUIRES
  set(requires_coreneuron OFF)
  foreach(required_feature ${NRN_ADD_TEST_REQUIRES})
    if(NOT DEFINED feature_${required_feature}_enabled)
      message(FATAL_ERROR "Unknown feature ${required_feature} used in REQUIRES expression")
    endif()
    if(NOT feature_${required_feature}_enabled)
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
      message(FATAL_ERROR "Unknown feature ${conflicting_feature} used in CONFLICTS expression")
    endif()
    if(feature_${conflicting_feature}_enabled)
      message(
        STATUS
          "Disabling ${NRN_ADD_TEST_GROUP}::${NRN_ADD_TEST_NAME}: ${conflicting_feature} enabled")
      return()
    endif()
  endforeach()
  # Get the prefix under which we stored information about this test group
  set(prefix NRN_TEST_GROUP_${NRN_ADD_TEST_GROUP})
  # Name of the target created by nrn_add_test_group that we should depend on. This might not exist
  # if the test group does not have any MOD files of its own.
  set(binary_target_name "${${prefix}_NRNIVMODL_TARGET_NAME}")
  if(TARGET "${binary_target_name}")
    # Directory where nrn_add_test_group ran nrnivmodl. If the target exists, this will be created.
    set(nrnivmodl_directory "${${prefix}_NRNIVMODL_DIRECTORY}")
  endif()
  # Path to the test repository.
  set(test_source_directory "${${prefix}_TEST_SOURCE_DIRECTORY}")
  # Get the variables that have global defaults but which can be overriden locally
  set(extra_environment "${${prefix}_DEFAULT_ENVIRONMENT}")
  set(output_files "${${prefix}_DEFAULT_OUTPUT}")
  set(script_patterns "${${prefix}_DEFAULT_SCRIPT_PATTERNS}")
  set(sim_directory "${${prefix}_DEFAULT_SIM_DIRECTORY}")
  # Override them locally if appropriate
  if(DEFINED NRN_ADD_TEST_ENVIRONMENT)
    set(extra_environment "${NRN_ADD_TEST_ENVIRONMENT}")
  endif()
  if(DEFINED NRN_ADD_TEST_OUTPUT)
    set(output_files "${NRN_ADD_TEST_OUTPUT}")
  endif()
  if(DEFINED NRN_ADD_TEST_SCRIPT_PATTERNS)
    set(script_patterns "${NRN_ADD_TEST_SCRIPT_PATTERNS}")
  endif()
  if(DEFINED NRN_ADD_TEST_SIM_DIRECTORY)
    set(sim_directory "${NRN_ADD_TEST_SIM_DIRECTORY}")
  endif()
  # Finally a working directory for this specific test within the group
  set(working_directory "${PROJECT_BINARY_DIR}/test/${NRN_ADD_TEST_GROUP}/${NRN_ADD_TEST_NAME}")
  file(MAKE_DIRECTORY "${working_directory}")
  if(DEFINED nrnivmodl_directory)
    execute_process(
      COMMAND
        ${CMAKE_COMMAND} -E create_symlink "${nrnivmodl_directory}/${CMAKE_HOST_SYSTEM_PROCESSOR}"
        "${working_directory}/${CMAKE_HOST_SYSTEM_PROCESSOR}")
  endif()
  # Set up the actual test. First, collect the script files that need to be copied into the test-
  # specific working directory and copy them there.
  foreach(script_pattern ${script_patterns})
    # We want to preserve directory structures, so if you pass SCRIPT_PATTERNS path/to/*.py then you
    # end up with {build_directory}/path/to/test_working_directory/path/to/script.py
    file(
      GLOB script_files
      RELATIVE "${test_source_directory}/${sim_directory}"
      "${test_source_directory}/${sim_directory}/${script_pattern}")
    foreach(script_file ${script_files})
      # We use NO_TARGET because otherwise we would in some cases generate a lot of
      # build-time-copy-{hash} top-level targets, which the Makefile build system struggles with.
      # Instead we make a single top-level target that depends on all scripts copied for this test.
      cpp_cc_build_time_copy(
        INPUT "${test_source_directory}/${sim_directory}/${script_file}"
        OUTPUT "${working_directory}/${script_file}"
        NO_TARGET)
      list(APPEND all_copied_script_files "${working_directory}/${script_file}")
    endforeach()
  endforeach()
  # Construct the name of the test and store it in a parent-scope list to be used when setting up
  # the comparison job
  set(test_name "${NRN_ADD_TEST_GROUP}::${NRN_ADD_TEST_NAME}")
  add_custom_target(copy-scripts-${NRN_ADD_TEST_GROUP}-${NRN_ADD_TEST_NAME} ALL
                    DEPENDS ${all_copied_script_files})
  # Depend on the target that runs nrnivmodl, if it exists. Otherwise it will not be run.
  if(TARGET ${binary_target_name})
    add_dependencies(copy-scripts-${NRN_ADD_TEST_GROUP}-${NRN_ADD_TEST_NAME} ${binary_target_name})
  endif()
  set(group_members "${${prefix}_TESTS}")
  list(APPEND group_members "${test_name}")
  set(${prefix}_TESTS
      "${group_members}"
      PARENT_SCOPE)
  set(test_env "${NRN_RUN_FROM_BUILD_DIR_ENV}")
  if(requires_coreneuron AND CORENRN_ENABLE_SHARED)
    # Did we run nrnivmodl specifically for this test, or does it just use default mechanisms?
    if(DEFINED nrnivmodl_directory)
      set(build_prefix "${nrnivmodl_directory}")
      set(mech_lib_name "corenrnmech")
    else()
      set(build_prefix "${CMAKE_BINARY_DIR}/bin")
      set(mech_lib_name "corenrnmech_internal")
    endif()
    list(
      APPEND
      test_env
      "CORENEURONLIB=${build_prefix}/${CMAKE_HOST_SYSTEM_PROCESSOR}/${CMAKE_SHARED_LIBRARY_PREFIX}${mech_lib_name}${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )
  endif()
  # Get [VAR1, VAR2, ...] from [VAR1=VAL1, VAR2=VAL2, ...]
  set(test_env_var_names ${test_env})
  list(TRANSFORM test_env_var_names REPLACE "^([^=]+)=.*$" "\\1")
  if(DEFINED nrnivmodl_directory)
    if(NOT "PATH" IN_LIST test_env_var_names)
      message(FATAL_ERROR "Expected to find PATH in ${test_env_var_names} but didn't")
    endif()
    # PATH will already be set in test_env
    list(TRANSFORM test_env REPLACE "^PATH="
                                    "PATH=${nrnivmodl_directory}/${CMAKE_HOST_SYSTEM_PROCESSOR}:")
  endif()
  list(TRANSFORM test_env REPLACE "^PYTHONPATH="
                                  "PYTHONPATH=${CMAKE_SOURCE_DIR}/docs/nmodl/python_scripts:")
  # Get the list of variables being set
  set(extra_env_var_names ${extra_environment})
  list(TRANSFORM extra_env_var_names REPLACE "^([^=]+)=.*$" "\\1")
  # Make sure the new variables don't overlap with the old ones; otherwise we'd need to do some
  # merging, which sounds hard in the general case.
  list(APPEND new_vars_being_set ${extra_env_var_names})
  list(REMOVE_ITEM new_vars_being_set ${test_env_var_names})
  if(NOT "${new_vars_being_set}" STREQUAL "${extra_env_var_names}")
    message(FATAL_ERROR "New (${extra_env_var_names}) vars overlap old (${test_env_var_names}). "
                        "This is not supported.")
  endif()
  list(APPEND test_env ${extra_environment})
  if(NRN_ADD_TEST_PRELOAD_SANITIZER AND NRN_SANITIZER_LIBRARY_PATH)
    list(APPEND test_env ${NRN_SANITIZER_PRELOAD_VAR}=${NRN_SANITIZER_LIBRARY_PATH})
    # On macOS with SIP then dynamic loader preload variables are not propagated to child processes.
    # By passing the key/value in our own private variables we make it easy to manually re-set the
    # preload variables in tests that spawn subprocesses. See also:
    # https://jonasdevlieghere.com/sanitizing-python-modules/ and
    # https://tobywf.com/2021/02/python-ext-asan/
    list(APPEND test_env NRN_SANITIZER_PRELOAD_VAR=${NRN_SANITIZER_PRELOAD_VAR})
    list(APPEND test_env NRN_SANITIZER_PRELOAD_VAL=${NRN_SANITIZER_LIBRARY_PATH})
    list(APPEND test_env NRN_PYTHON_EXECUTABLE=${NRN_DEFAULT_PYTHON_EXECUTABLE})
  endif()
  list(APPEND test_env ${NRN_SANITIZER_ENABLE_ENVIRONMENT})
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
    COMMAND ${CMAKE_COMMAND} -E env ${test_env} ${NRN_ADD_TEST_COMMAND}
    WORKING_DIRECTORY "${working_directory}")
  set(test_names ${test_name})
  if(NRN_ADD_TEST_PRECOMMAND)
    add_test(
      NAME ${test_name}::preparation
      COMMAND ${CMAKE_COMMAND} -E env ${test_env} ${NRN_ADD_TEST_PRECOMMAND}
      WORKING_DIRECTORY "${working_directory}")
    list(APPEND test_names ${test_name}::preparation)
    set_tests_properties(${test_name} PROPERTIES DEPENDS ${test_name}::preparation)
  endif()
  set_tests_properties(${test_names} PROPERTIES TIMEOUT 500)
  if(DEFINED NRN_ADD_TEST_PROCESSORS)
    set_tests_properties(${test_names} PROPERTIES PROCESSORS ${NRN_ADD_TEST_PROCESSORS})
  endif()
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
    cpp_cc_build_time_copy(INPUT "${PROJECT_SOURCE_DIR}/${reference_path}"
                           OUTPUT "${test_directory}/${reference_path}")
  endforeach()

  # Copy the comparison script
  cpp_cc_build_time_copy(INPUT "${PROJECT_SOURCE_DIR}/test/scripts/compare_test_results.py"
                         OUTPUT "${test_directory}/compare_test_results.py")

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
