set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(${NRN_INCLUDE_DIRS})

# Add directory-level default compiler flags -- these should be added to all NEURON targets, but not
# targets from included projects like CoreNEURON and NMODL
add_compile_options(${NRN_COMPILE_FLAGS})
add_compile_definitions(${NRN_COMPILE_DEFS})
add_link_options(${NRN_LINK_FLAGS})
include(NeuronTestHelper)

# =============================================================================
# Test executables
# =============================================================================
add_executable(
  testneuron
  common/catch2_main.cpp
  unit_tests/basic.cpp
  unit_tests/iovec.cpp
  unit_tests/container/container.cpp
  unit_tests/container/generic_data_handle.cpp
  unit_tests/container/mechanism.cpp
  unit_tests/container/node.cpp
  unit_tests/utils/enumerate.cpp
  unit_tests/oc/hoc_interpreter.cpp)
set(catch2_targets testneuron)
if(NRN_ENABLE_THREADS)
  add_executable(nrn-benchmarks common/catch2_main.cpp benchmarks/threads/test_multicore.cpp)
  target_link_libraries(nrn-benchmarks Threads::Threads)
  list(APPEND catch2_targets nrn-benchmarks)
endif()
foreach(target ${catch2_targets})
  cpp_cc_configure_sanitizers(TARGET ${target})
  target_link_libraries(${target} Catch2::Catch2 nrniv_lib)
  if(NOT MINGW)
    target_link_libraries(${target} ${CMAKE_DL_LIBS})
  endif()
endforeach()

# =============================================================================
# Copy necessary hoc files to build directory if they have not been copied yet
# =============================================================================
add_dependencies(testneuron copy_share_lib_to_build)

# =============================================================================
# Unit tests  (testneuron, ...)
# =============================================================================
nrn_add_test_group(NAME unit_tests MODFILE_PATTERNS NONE)
nrn_add_test(
  GROUP unit_tests
  NAME testneuron
  COMMAND $<TARGET_FILE:testneuron>)
# Extra tests from testneuron that need manual checking because they call std::terminate.
nrn_add_test(
  GROUP unit_tests
  NAME testneuron_soa_erase_calls_terminate
  COMMAND $<TARGET_FILE:testneuron>
          "Deleting a row from a frozen SoA container causes a fatal error")
string(
  JOIN " "
  regex "neuron::container::owning_identifier<neuron::container::Node::storage> destructor could"
        "not delete from the underlying storage:"
        "neuron::container::Node::storage\\[frozen_count=1,sorted=false\\]: erase\\(\\) called on a"
        "frozen structure \\[std::runtime_error\\]\\. This is not recoverable, aborting\\.")
set_tests_properties(unit_tests::testneuron_soa_erase_calls_terminate
                     PROPERTIES PASS_REGULAR_EXPRESSION "${regex}")
if(TARGET nrn-benchmarks AND NRN_ENABLE_PERFORMANCE_TESTS)
  nrn_add_test(
    GROUP unit_tests
    NAME benchmarks
    PROCESSORS 16
    COMMAND $<TARGET_FILE:nrn-benchmarks> --processors=16)
endif()

# =============================================================================
# Add ringtest
# =============================================================================
set(RINGTEST_DIR ${PROJECT_SOURCE_DIR}/test/ringtest)
add_test(
  ringtest
  ${CMAKE_COMMAND}
  -Dexecutable=${CMAKE_BINARY_DIR}/bin/nrniv
  -Dexec_arg=ring.hoc
  -Dout_file=out.dat
  -Dref_file=out.dat.ref
  -Dwork_dir=${RINGTEST_DIR}
  -P
  ${PROJECT_SOURCE_DIR}/cmake/RunHOCTest.cmake)
list(APPEND TESTS ringtest)

# =============================================================================
# Add small hoc test
# =============================================================================
set(HOCTEST_DIR ${PROJECT_SOURCE_DIR}/test/hoc_tests/connect_dend)
if(NRN_ENABLE_CORENEURON OR NRN_ENABLE_MOD_COMPATIBILITY)
  set(REF_FILE cell3soma.core.dat.ref)
else()
  set(REF_FILE cell3soma.dat.ref)
endif()
add_test(
  connect_dend
  ${CMAKE_COMMAND}
  -Dexecutable=${CMAKE_BINARY_DIR}/bin/nrniv
  -Dexec_arg=connect_dend.hoc
  -Dout_file=cell3soma.dat
  -Dref_file=${REF_FILE}
  -Dwork_dir=${HOCTEST_DIR}
  -P
  ${PROJECT_SOURCE_DIR}/cmake/RunHOCTest.cmake)
list(APPEND TESTS connect_dend)

# =============================================================================
# Check if --oversubscribe is a valid option for mpiexec
# =============================================================================
if(NRN_ENABLE_MPI)
  # Detect if the MPI implementation supports the --oversubscribe option (at the time of writing the
  # available version of OpenMPI does but those of HPE-MPI and MPICH do not).
  set(MPIEXEC_OVERSUBSCRIBE --oversubscribe)
  execute_process(
    COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_OVERSUBSCRIBE} --version
    RESULT_VARIABLE MPIEXEC_OVERSUBSCRIBE_TEST
    OUTPUT_QUIET ERROR_QUIET)
  if(NOT MPIEXEC_OVERSUBSCRIBE_TEST EQUAL 0)
    message(STATUS "mpiexec does not support ${MPIEXEC_OVERSUBSCRIBE}")
    unset(MPIEXEC_OVERSUBSCRIBE)
  endif()
  if(APPLE
     AND NRN_ENABLE_PYTHON
     AND NRN_SANITIZERS)
    # Detect how to tell ${MPIEXEC_EXECUTABLE} to set extra environment variables when launching the
    # child process. This is important for using the sanitizers on macOS, as there variables like
    # DYLD_INSERT_LIBRARIES are not propagated from parent processes to their children.
    # ~~~
    # -env works with mpich, -genv works with HPE-MPI, -x works with OpenMPI
    foreach(option
            "-env;NRN_PROBE_VARIABLE=NRN_PROBE_VALUE" "-genv;NRN_PROBE_VARIABLE;NRN_PROBE_VALUE"
            "-x;NRN_PROBE_VARIABLE=NRN_PROBE_VALUE")
      set(probe_command
          ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1 ${MPIEXEC_PREFLAGS} ${option}
          ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${MPIEXEC_POSTFLAGS} -c
          "import os\; print(os.environ['NRN_PROBE_VARIABLE'])")
      execute_process(
        COMMAND ${probe_command}
        RESULT_VARIABLE mpiexec_env_test
        OUTPUT_VARIABLE mpiexec_env_test_stdout
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE)
      if(mpiexec_env_test EQUAL 0 AND mpiexec_env_test_stdout STREQUAL "NRN_PROBE_VALUE")
        set(preload_sanitizer_mpiexec "${option}")
        string(REPLACE "NRN_PROBE_VARIABLE" "${NRN_SANITIZER_PRELOAD_VAR}"
                       preload_sanitizer_mpiexec "${preload_sanitizer_mpiexec}")
        string(REPLACE "NRN_PROBE_VALUE" "${NRN_SANITIZER_LIBRARY_PATH}" preload_sanitizer_mpiexec
                       "${preload_sanitizer_mpiexec}")
        break()
      endif()
    endforeach()
    if(NOT DEFINED preload_sanitizer_mpiexec)
      message(WARNING "Could not detect how to preload sanitizer runtimes under mpiexec.")
      message(WARNING "You may see failures in tests launched as mpiexec ... python ...")
    endif()
  endif()
endif()

nrn_add_test_group(NAME mpi_init MODFILE_PATTERNS NONE)
set(mpi_init_requires_mpiexec mpi)
set(mpi_init_prefix_mpiexec ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 1 ${MPIEXEC_PREFLAGS})
set(mpi_init_suffix_mpiexec ${MPIEXEC_POSTFLAGS})
foreach(mpiexec "" "_mpiexec")
  set(nrniv ${mpi_init_prefix${mpiexec}} nrniv ${mpi_init_suffix${mpiexec}})
  nrn_add_test(
    GROUP mpi_init
    NAME nrniv${mpiexec}_mpiopt
    REQUIRES ${mpi_init_requires${mpiexec}}
    COMMAND ${nrniv} -mpi -c "quit()")
  nrn_add_test(
    GROUP mpi_init
    NAME nrniv${mpiexec}_nrnmpi_init
    REQUIRES ${mpi_init_requires${mpiexec}}
    COMMAND ${nrniv} -c "nrnmpi_init()" -c "quit()")
  # See https://www.neuron.yale.edu/phpBB/viewtopic.php?t=4297 and links therein.
  if(NRN_HAVE_OPENMPI2_OR_LESS AND NOT NRN_ENABLE_MPI_DYNAMIC)
    set(python ${mpi_init_prefix${mpiexec}} nrniv ${mpi_init_suffix${mpiexec}} -python)
  else()
    set(python ${mpi_init_prefix${mpiexec}} ${preload_sanitizer${mpiexec}}
               ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${mpi_init_suffix${mpiexec}})
  endif()
  nrn_add_test(
    GROUP mpi_init
    NAME python${mpiexec}_nrnmpi_init
    PRELOAD_SANITIZER
    REQUIRES python ${mpi_init_requires${mpiexec}}
    COMMAND ${python} -c "from neuron import h$<SEMICOLON> h.nrnmpi_init()$<SEMICOLON> h.quit()")
  nrn_add_test(
    GROUP mpi_init
    NAME python${mpiexec}_mpienv
    PRELOAD_SANITIZER
    REQUIRES python ${mpi_init_requires${mpiexec}}
    ENVIRONMENT NEURON_INIT_MPI=1
    COMMAND ${python} -c "from neuron import h$<SEMICOLON> h.quit()")
endforeach()

# =============================================================================
# Add pytest
# =============================================================================
if(NRN_ENABLE_PYTHON)
  # use `--capture=tee-sys`: combines 'sys' and '-s', capturing sys.stdout/stderr and passing it
  # along to the actual sys.stdout/stderr
  set(pytest_args --capture=tee-sys)
  # pytest-cov runs extremely slowly under AddressSanitizer
  if(PYTEST_COV_FOUND AND NOT "address" IN_LIST NRN_SANITIZERS_LIST)
    list(APPEND pytest_args --cov-report=xml --cov=neuron)
  endif()
  set(pytest -m pytest ${pytest_args})

  nrn_add_test(
    GROUP unit_tests
    NAME python_unit_tests
    COMMAND ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${pytest}
            ${PROJECT_SOURCE_DIR}/test/unit_tests/hoc_python
    PRELOAD_SANITIZER
    SCRIPT_PATTERNS "test/unit_tests/hoc_python/*.py")

  # In brief:
  #
  # * tests and mod files under pytest/ are only compatible with NEURON
  # * tests and mod files under pytest_coreneuron/ are compatible with both NEURON and CoreNEURON
  #
  # So, pytest/ should generally be viewed as a staging area en route to pytest_coreneuron. As the
  # name implies, tests in these directories are executed using pytest.
  nrn_add_test_group(NAME pytest MODFILE_PATTERNS test/pytest/*.mod)
  nrn_add_test_group(
    CORENEURON
    NAME pytest_coreneuron
    MODFILE_PATTERNS test/pytest_coreneuron/*.mod)
  # In case of multiple Python versions, run these tests with all of them
  foreach(val RANGE ${NRN_PYTHON_ITERATION_LIMIT})
    list(GET NRN_PYTHON_EXECUTABLES ${val} exe)
    list(GET NRN_PYTHON_VERSIONS ${val} pyver)
    foreach(group pytest pytest_coreneuron)
      nrn_add_test(
        GROUP ${group}
        NAME basic_tests_py${pyver}
        PRELOAD_SANITIZER
        COMMAND "${exe}" ${pytest} "./test/${group}"
        SCRIPT_PATTERNS "test/${group}/*.json" "test/${group}/*.py")
    endforeach()
  endforeach()

  # Add some tests that are specifically aimed at NEURON - Python integration, covering different
  # ways NEURON can be made to execute Python code.
  add_subdirectory(pyinit)

  nrn_add_test_group(NAME datahandle MODFILE_PATTERNS test/datahandle/*.mod)
  nrn_add_test(
    GROUP datahandle
    NAME datahandle_tests
    PRELOAD_SANITIZER
    COMMAND ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${pytest} ./test/datahandle
    SCRIPT_PATTERNS test/datahandle/*.py)

  # Mostly to increase coverage
  nrn_add_test_group(NAME coverage_tests MODFILE_PATTERNS test/cover/mod/*.mod)
  nrn_add_test(
    GROUP coverage_tests
    NAME cover_tests
    PRELOAD_SANITIZER
    ENVIRONMENT COVERAGE_FILE=.coverage.cover_tests
    COMMAND ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${pytest} ./test/cover
    SCRIPT_PATTERNS test/cover/*.py test/cover/*.json)
  nrn_add_test_group(
    NAME example_nmodl
    MODFILE_PATTERNS *.mod *.inc
    SIM_DIRECTORY share/examples/nrniv/nmodl)
  set(py_exe ${NRN_DEFAULT_PYTHON_EXECUTABLE})
  set(py_preload PRELOAD_SANITIZER)
  set(hoc_exe special)
  foreach(ext hoc py)
    file(
      GLOB example_nmodl_scripts
      RELATIVE "${PROJECT_SOURCE_DIR}/share/examples/nrniv/nmodl"
      "${PROJECT_SOURCE_DIR}/share/examples/nrniv/nmodl/*.${ext}")
    foreach(example_script ${example_nmodl_scripts})
      get_filename_component(name "${example_script}" NAME_WLE)
      nrn_add_test(
        GROUP example_nmodl
        NAME ${name}_${ext} ${${ext}_preload} # PRELOAD_SANITIZER for Python
        COMMAND ${${ext}_exe} "${example_script}"
        SCRIPT_PATTERNS "${example_script}" "${name}.ses")
    endforeach()
  endforeach()

  # independent run of each hoc and python file in test/hoc/* folders
  nrn_add_test_group(
    NAME hoctests
    MODFILE_PATTERNS *.mod *.inc
    SIM_DIRECTORY test/hoctests)
  set(hoctest_utils expect_err.hoc)
  foreach(ext hoc py)
    file(
      GLOB hoc_scripts
      RELATIVE "${PROJECT_SOURCE_DIR}/test/hoctests"
      "${PROJECT_SOURCE_DIR}/test/hoctests/*/*.${ext}")
    foreach(hoc_script ${hoc_scripts})
      get_filename_component(name "${hoc_script}" NAME_WLE)
      nrn_add_test(
        GROUP hoctests
        NAME ${name}_${ext} ${${ext}_preload} # PRELOAD_SANITIZER for Python
        COMMAND ${${ext}_exe} "${hoc_script}"
        SCRIPT_PATTERNS "${hoc_script}" "tests/${name}.json" ${${ext}test_utils})
    endforeach()
  endforeach()

  if(NRN_ENABLE_RX3D)
    nrn_add_test_group(
      NAME rxdmod_tests
      MODFILE_PATTERNS test/rxd/*.mod
      SCRIPT_PATTERNS test/rxd/*.py test/rxd/*/*.py test/rxd/3d/*.asc test/rxd/testdata/**/*.dat)
    # These tests include comparisons against saved data, which appears to have been generated using
    # GCC. Other compilers produce larger differences.
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
      set(change_test_tolerance NRN_RXD_TEST_TOLERANCE=1e-8)
    elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "IntelLLVM")
      set(change_test_tolerance NRN_RXD_TEST_TOLERANCE=3e-8)
    elseif(NRN_HAVE_NVHPC_COMPILER)
      set(change_test_tolerance NRN_RXD_TEST_TOLERANCE=1e-4)
    endif()
    nrn_add_test(
      GROUP rxdmod_tests
      NAME rxd_tests
      PRELOAD_SANITIZER
      ENVIRONMENT COVERAGE_FILE=.coverage.rxd_tests ${change_test_tolerance}
      COMMAND ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${pytest} ./test/rxd)
    if(NRN_ENABLE_MPI)
      nrn_find_python_module(MODULE mpi4py)
      if(mpi4py_FOUND)
        nrn_add_test(
          GROUP rxdmod_tests
          NAME rxd_mpi_tests
          PRELOAD_SANITIZER
          ENVIRONMENT COVERAGE_FILE=.coverage.rxd_mpi_tests ${change_test_tolerance}
          COMMAND
            ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 1 ${MPIEXEC_PREFLAGS}
            ${preload_sanitizer_mpiexec} ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${MPIEXEC_POSTFLAGS}
            ${pytest} ./test/rxd --mpi)
      endif()
    endif()
  endif()
  nrn_add_test_group(NAME parallel MODFILE_PATTERNS test/pytest_coreneuron/*.mod)
  nrn_add_test(
    GROUP parallel
    NAME subworld
    PROCESSORS 6
    REQUIRES mpi
    SCRIPT_PATTERNS test/parallel_tests/test_subworld.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            nrniv ${MPIEXEC_POSTFLAGS} -mpi -python test/parallel_tests/test_subworld.py)
  nrn_add_test(
    GROUP parallel
    NAME partrans
    PROCESSORS 2
    REQUIRES mpi
    SCRIPT_PATTERNS test/pytest_coreneuron/test_partrans.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            nrniv ${MPIEXEC_POSTFLAGS} -mpi -python test/pytest_coreneuron/test_partrans.py)
  nrn_add_test(
    GROUP parallel
    NAME netpar
    PROCESSORS 2
    REQUIRES mpi
    SCRIPT_PATTERNS test/pytest_coreneuron/test_hoc_po.py test/pytest_coreneuron/test_netpar.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            nrniv ${MPIEXEC_POSTFLAGS} -mpi -python test/pytest_coreneuron/test_netpar.py)
  nrn_add_test(
    GROUP parallel
    NAME bas
    PROCESSORS 2
    REQUIRES mpi
    SCRIPT_PATTERNS test/parallel_tests/test_bas.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            nrniv ${MPIEXEC_POSTFLAGS} -mpi -python test/parallel_tests/test_bas.py)
  # TODO if we need to pass more complicated argument strings then some smarter escaping will be
  # needed
  string(JOIN " " pytest_arg_string ${pytest_args})
  nrn_add_test(
    GROUP parallel
    NAME nrntest_fast
    PROCESSORS 2
    REQUIRES mpi
    ENVIRONMENT "NRN_PYTEST_ARGS=${pytest_arg_string}"
    SCRIPT_PATTERNS
      test/pytest_coreneuron/run_pytest.py test/pytest_coreneuron/test_nrntest_fast.json
      test/pytest_coreneuron/test_nrntest_fast.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            special ${MPIEXEC_POSTFLAGS} -mpi -python test/pytest_coreneuron/run_pytest.py)
  # CoreNEURON's reports require MPI and segfault if it is not initialised. This is a crude
  # workaround.
  if(CORENRN_ENABLE_REPORTING)
    set(nrniv_mpi_arg -mpi)
    set(nrnpython_mpi_env NEURON_INIT_MPI=1)
    # see #1619, required when coreneuron is linked to sonata library
    set(sonata_zero_gid_env LIBSONATA_ZERO_BASED_GIDS=1)
  endif()
  # If CoreNEURON is linked statically then it cannot be dynamically loaded by Python or nrniv, so
  # we need to launch tests with `special [-python]`. If you consider changing how
  # ${modtests_launch_py_mpi} is constructed (notably the use of ${MPIEXEC_NAME} instead of
  # ${MPIEXEC}) then first refer to GitHub issue #894 and note that nrn_add_test prefixes the
  # command with ${CMAKE_COMMAND} -E env.
  if(NOT CORENRN_ENABLE_SHARED)
    # ${nrniv_mpi_arg} is not enough here, it crashes without -mpi even if CoreNEURON reports are
    # disabled.
    set(modtests_launch_hoc special -notatty -mpi)
    set(modtests_launch_py ${modtests_launch_hoc} -python)
    set(modtests_launch_py_mpi
        ${MPIEXEC_NAME}
        ${MPIEXEC_NUMPROC_FLAG}
        2
        ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS}
        special
        ${MPIEXEC_POSTFLAGS}
        -notatty
        -python)
    set(modtests_launch_py_mpi_subworlds
        ${MPIEXEC_NAME}
        ${MPIEXEC_NUMPROC_FLAG}
        6
        ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS}
        special
        ${MPIEXEC_POSTFLAGS}
        -notatty
        -python)
  else()
    set(modtests_preload_sanitizer PRELOAD_SANITIZER)
    set(modtests_launch_py ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${pytest})
    set(modtests_launch_hoc nrniv ${nrniv_mpi_arg})
    set(modtests_launch_py_mpi
        ${MPIEXEC_NAME}
        ${MPIEXEC_NUMPROC_FLAG}
        2
        ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS}
        ${preload_sanitizer_mpiexec}
        ${NRN_DEFAULT_PYTHON_EXECUTABLE}
        ${MPIEXEC_POSTFLAGS})
    set(modtests_launch_py_mpi_subworlds
        ${MPIEXEC_NAME}
        ${MPIEXEC_NUMPROC_FLAG}
        6
        ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS}
        ${preload_sanitizer_mpiexec}
        ${NRN_DEFAULT_PYTHON_EXECUTABLE}
        ${MPIEXEC_POSTFLAGS})
  endif()

  # This test uses the standard NEURON installation (without nrnivmodl having been run)
  if(CORENRN_ENABLE_SHARED)
    nrn_add_test_group(
      CORENEURON
      NAME coreneuron_standalone
      MODFILE_PATTERNS NONE)

    nrn_add_test(
      GROUP coreneuron_standalone
      NAME test_nrn_corenrn_standalone
      REQUIRES coreneuron ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_psolve.py
      ENVIRONMENT COVERAGE_FILE=.coverage.coreneuron_standalone_test_psolve_py
                  ${sonata_zero_gid_env} ${nrnpython_mpi_env}
      COMMAND ${modtests_launch_py} test/coreneuron/test_psolve.py)
  endif()

  nrn_add_test_group(
    NAME nmodl_tests
    SCRIPT_PATTERNS test/nmodl/*.py
    MODFILE_PATTERNS "docs/nmodl/mod/*.mod")
  nrn_add_test(
    GROUP nmodl_tests
    NAME test_table
    REQUIRES ${modtests_preload_sanitizer}
    SCRIPT_PATTERNS test/nmodl/test_table.py
    ENVIRONMENT ${sonata_zero_gid_env} ${nrnpython_mpi_env}
    COMMAND ${modtests_launch_py} test/nmodl/test_table.py)
  nrn_add_test(
    GROUP nmodl_tests
    NAME test_disc
    REQUIRES ${modtests_preload_sanitizer}
    SCRIPT_PATTERNS test/nmodl/test_disc.py
    ENVIRONMENT ${sonata_zero_gid_env} ${nrnpython_mpi_env}
    COMMAND ${modtests_launch_py} test/nmodl/test_disc.py)
  nrn_add_test(
    GROUP nmodl_tests
    NAME test_function_table
    REQUIRES ${modtests_preload_sanitizer}
    SCRIPT_PATTERNS test/nmodl/test_function_table.py
    ENVIRONMENT ${sonata_zero_gid_env} ${nrnpython_mpi_env}
    COMMAND ${modtests_launch_py} test/nmodl/test_function_table.py)
  nrn_add_test(
    GROUP nmodl_tests
    NAME test_kinetic
    REQUIRES ${modtests_preload_sanitizer}
    SCRIPT_PATTERNS test/nmodl/test_kinetic.py
    ENVIRONMENT ${sonata_zero_gid_env} ${nrnpython_mpi_env}
    COMMAND ${modtests_launch_py} test/nmodl/test_kinetic.py)
  nrn_add_test(
    GROUP nmodl_tests
    NAME test_random
    REQUIRES ${modtests_preload_sanitizer}
    SCRIPT_PATTERNS test/nmodl/test_random.py
    ENVIRONMENT ${sonata_zero_gid_env} ${nrnpython_mpi_env}
    COMMAND ${modtests_launch_py} test/nmodl/test_random.py)

  nrn_add_test_group(
    CORENEURON
    NAME coreneuron_modtests
    # This get used in 4 tests so make it the default and override in other tests.
    SCRIPT_PATTERNS test/coreneuron/test_spikes.py
    MODFILE_PATTERNS
      "test/coreneuron/mod files/*.mod" "test/coreneuron/mod files/axial.inc"
      test/pytest_coreneuron/unitstest.mod test/pytest_coreneuron/version_macros.mod
      test/gjtests/natrans.mod)
  nrn_add_test(
    GROUP coreneuron_modtests
    NAME version_macros
    REQUIRES coreneuron ${modtests_preload_sanitizer}
    SCRIPT_PATTERNS test/pytest_coreneuron/test_version_macros.py
    ENVIRONMENT COVERAGE_FILE=.coverage.coreneuron_version_macros NRN_CORENEURON_ENABLE=true
                ${sonata_zero_gid_env} ${nrnpython_mpi_env}
    COMMAND ${modtests_launch_py} test/pytest_coreneuron/test_version_macros.py)
  nrn_add_test(
    GROUP coreneuron_modtests
    NAME inputpresyn_py
    REQUIRES coreneuron mpi ${modtests_preload_sanitizer}
    SCRIPT_PATTERNS test/coreneuron/test_inputpresyn.py
    PROCESSORS 2
    COMMAND ${modtests_launch_py_mpi} test/coreneuron/test_inputpresyn.py)
  # In GPU builds run all of the tests on both CPU and GPU
  set(coreneuron_modtests_gpu_env CORENRN_ENABLE_GPU=true)
  foreach(processor cpu gpu)
    set(processor_env ${coreneuron_modtests_${processor}_env})
    set(modtests_processor_env ${processor_env} ${sonata_zero_gid_env})
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME fornetcon_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_fornetcon.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_fornetcon_py
      COMMAND ${modtests_launch_py} test/coreneuron/test_fornetcon.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME direct_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_direct.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_direct_py
      COMMAND ${modtests_launch_py} test/coreneuron/test_direct.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME direct_hoc_${processor}
      REQUIRES coreneuron ${processor}
      SCRIPT_PATTERNS test/coreneuron/test_direct.hoc
      ENVIRONMENT ${processor_env}
      COMMAND ${modtests_launch_hoc} test/coreneuron/test_direct.hoc)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME spikes_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_spikes_py
      COMMAND ${modtests_launch_py} test/coreneuron/test_spikes.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME spikes_file_mode_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_spikes_file_mode_py NRN_TEST_SPIKES_FILE_MODE=1
      COMMAND ${modtests_launch_py} test/coreneuron/test_spikes.py)
    # TODO this test does some NEURON vs. NEURON testing too
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME fast_imem_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/pytest_coreneuron/test_fast_imem.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_fast_imem_py
      COMMAND ${modtests_launch_py} test/pytest_coreneuron/test_fast_imem.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME datareturn_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_datareturn.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_datareturn_py
      COMMAND ${modtests_launch_py} test/coreneuron/test_datareturn.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_units_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_units.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_test_units_py
      COMMAND ${modtests_launch_py} test/coreneuron/test_units.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_netmove_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_netmove.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_test_netmove_py
      COMMAND ${modtests_launch_py} test/coreneuron/test_netmove.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_pointer_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_pointer.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_test_pointer_py
      COMMAND ${modtests_launch_py} test/coreneuron/test_pointer.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_watchrange_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_watchrange.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_test_watchrange_py
      COMMAND ${modtests_launch_py} test/coreneuron/test_watchrange.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_psolve_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_psolve.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_test_psolve_py
      COMMAND ${modtests_launch_py} test/coreneuron/test_psolve.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_ba_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_ba.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_test_ba_py
      COMMAND ${modtests_launch_py} test/coreneuron/test_ba.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_nmodlrandom_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_nmodlrandom.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_test_nmodlrandom_py
      COMMAND ${modtests_launch_py} test/coreneuron/test_nmodlrandom.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_nmodlrandom_syntax_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_nmodlrandom_syntax.py
      ENVIRONMENT
        ${modtests_processor_env} ${nrnpython_mpi_env}
        COVERAGE_FILE=.coverage.coreneuron_test_nmodlrandom_syntax_py
        NMODL_BINARY=${CORENRN_NMODL_BINARY}
      COMMAND ${modtests_launch_py} test/coreneuron/test_nmodlrandom_syntax.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_natrans_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/gjtests/test_natrans.py
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                  COVERAGE_FILE=.coverage.coreneuron_test_natrans_py
      COMMAND ${modtests_launch_py} test/gjtests/test_natrans.py)
    if(NRN_ENABLE_MPI)
      nrn_find_python_module(MODULE mpi4py)
      if(mpi4py_FOUND)
        nrn_add_test(
          GROUP coreneuron_modtests
          NAME spikes_mpi_py_${processor}
          REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
          PROCESSORS 2
          ENVIRONMENT ${processor_env} NRN_TEST_SPIKES_MPI4PY=1
          COMMAND ${modtests_launch_py_mpi} test/coreneuron/test_spikes.py)
      endif()
      # See https://www.neuron.yale.edu/phpBB/viewtopic.php?t=4297 and links therein for more
      # discussion. Launching this test via `python` (not `special`) does not work on Linux with
      # OpenMPI 2.x and without dynamic MPI enabled in NEURON, which corresponds to the GitHub
      # Actions environment. GPU builds are already launched using `special`.
      if(NOT NRN_ENABLE_MPI_DYNAMIC
         AND NOT CORENEURON_ENABLE_GPU
         AND NRN_HAVE_OPENMPI2_OR_LESS)
        set(launch_spikes_mpi_file_mode_py
            ${MPIEXEC_NAME}
            ${MPIEXEC_NUMPROC_FLAG}
            2
            ${MPIEXEC_OVERSUBSCRIBE}
            ${MPIEXEC_PREFLAGS}
            special
            ${MPIEXEC_POSTFLAGS}
            -python)
        message(STATUS "Hitting workaround")
      else()
        set(launch_spikes_mpi_file_mode_py ${modtests_launch_py_mpi})
      endif()
      nrn_add_test(
        GROUP coreneuron_modtests
        NAME spikes_mpi_file_mode_py_${processor}
        REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
        PROCESSORS 2
        ENVIRONMENT ${processor_env} NRN_TEST_SPIKES_NRNMPI_INIT=1 NRN_TEST_SPIKES_FILE_MODE=1
        COMMAND ${launch_spikes_mpi_file_mode_py} test/coreneuron/test_spikes.py)
      nrn_add_test(
        GROUP coreneuron_modtests
        NAME test_subworlds_py_${processor}
        REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
        SCRIPT_PATTERNS test/coreneuron/test_subworlds.py
        PROCESSORS 6
        ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
                    COVERAGE_FILE=.coverage.coreneuron_test_subworlds_py
        COMMAND ${modtests_launch_py_mpi_subworlds} test/coreneuron/test_subworlds.py)
    endif()
    nrn_add_test_group(
      NAME nmodl_tests_coreneuron
      SCRIPT_PATTERNS test/nmodl/*.py
      MODFILE_PATTERNS "docs/nmodl/mod/*.mod")
    nrn_add_test(
      GROUP nmodl_tests_coreneuron
      NAME test_table_coreneuron_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/nmodl/test_table.py
      ENVIRONMENT ${sonata_zero_gid_env} ${nrnpython_mpi_env} ${processor_env}
      COMMAND ${modtests_launch_py} test/nmodl/test_table.py)
    nrn_add_test(
      GROUP nmodl_tests_coreneuron
      NAME test_disc_coreneuron_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/nmodl/test_disc.py
      ENVIRONMENT ${sonata_zero_gid_env} ${nrnpython_mpi_env} ${processor_env}
      COMMAND ${modtests_launch_py} test/nmodl/test_disc.py)
    nrn_add_test(
      GROUP nmodl_tests_coreneuron
      NAME test_function_table_coreneuron_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/nmodl/test_function_table.py
      ENVIRONMENT ${sonata_zero_gid_env} ${nrnpython_mpi_env} ${processor_env}
      COMMAND ${modtests_launch_py} test/nmodl/test_function_table.py)
    nrn_add_test(
      GROUP nmodl_tests_coreneuron
      NAME test_kinetic_coreneuron_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/nmodl/test_kinetic.py
      ENVIRONMENT ${sonata_zero_gid_env} ${nrnpython_mpi_env} ${processor_env}
      COMMAND ${modtests_launch_py} test/nmodl/test_kinetic.py)
  endforeach()

  if(NRN_ENABLE_MUSIC)
    set(music_launch_command
        "${MPIEXEC_NAME}  ${MPIEXEC_NUMPROC_FLAG}  2 ${MPIEXEC_OVERSUBSCRIBE}   ${MPIEXEC_PREFLAGS} music"
    )
    nrn_add_test_group(
      NAME nrnmusic
      ENVIRONMENT MUSIC_LIBDIR=${MUSIC_LIBDIR}
      ENVIRONMENT MPIEXEC_COMMAND=${music_launch_command}
      MODFILE_PATTERNS NONE)
    nrn_add_test(
      GROUP nrnmusic
      NAME music_tests
      PROCESSORS 2
      COMMAND ${NRN_DEFAULT_PYTHON_EXECUTABLE} test/music_tests/runtests.py
      SCRIPT_PATTERNS test/music_tests/*.music test/music_tests/*.py)
  endif()
endif()

# ============================================
# Test modlunit
# ============================================
function(add_modlunit_test mod_file)
  get_filename_component(mod_file_basename "${mod_file}" NAME_WE)
  add_test(
    NAME modlunit_${mod_file_basename}
    COMMAND modlunit "${mod_file}"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  list(APPEND TESTS modlunit_${mod_file_basename})
  set(TESTS
      "${TESTS}"
      PARENT_SCOPE)
endfunction()
add_modlunit_test("${PROJECT_SOURCE_DIR}/test/pytest_coreneuron/unitstest.mod")
add_modlunit_test("${PROJECT_SOURCE_DIR}/src/nrnoc/hh.mod")
add_modlunit_test("${PROJECT_SOURCE_DIR}/src/nrnoc/stim.mod")
add_modlunit_test("${PROJECT_SOURCE_DIR}/src/nrnoc/pattern.mod")
add_modlunit_test("${PROJECT_SOURCE_DIR}/test/hoctests/rand.mod")
set_property(TEST modlunit_pattern PROPERTY WILL_FAIL ON)

set_tests_properties(${TESTS} PROPERTIES ENVIRONMENT "${NRN_RUN_FROM_BUILD_DIR_ENV}")
cpp_cc_configure_sanitizers(TEST ${TESTS})
# If profiling is enabled, run ringtest with profiler
if(NRN_ENABLE_PROFILING)
  set(TEST_PROFILING_ENV ${NRN_RUN_FROM_BUILD_DIR_ENV})
  list(APPEND TEST_PROFILING_ENV "CALI_CONFIG=runtime-report,calc.inclusive")
  set_tests_properties(ringtest PROPERTIES ENVIRONMENT "${TEST_PROFILING_ENV}")
endif()

# Add tests that are configured using external repositories
add_subdirectory(external)
