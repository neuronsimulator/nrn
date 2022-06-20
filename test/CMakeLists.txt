set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(${PROJECT_SOURCE_DIR}/test ${PROJECT_SOURCE_DIR}/src/nrnoc
                    ${PROJECT_SOURCE_DIR}/src/nrniv ${PROJECT_SOURCE_DIR}/src/oc)

# Add directory-level default compiler flags -- these should be added to all NEURON targets, but not
# targets from included projects like CoreNEURON and NMODL
add_compile_options(${NRN_COMPILE_FLAGS})
add_compile_definitions(${NRN_COMPILE_DEFS})
add_link_options(${NRN_LINK_FLAGS})

# =============================================================================
# Test executables
# =============================================================================
set(TEST_SOURCES unit_tests/oc/hoc_interpreter.cpp)
add_executable(testneuron unit_tests/unit_test.cpp ${TEST_SOURCES})
cpp_cc_configure_sanitizers(TARGET testneuron)
target_compile_features(testneuron PUBLIC cxx_std_11)
target_link_libraries(testneuron Catch2::Catch2 nrniv_lib)
if(NOT MINGW)
  target_link_libraries(testneuron ${CMAKE_DL_LIBS})
endif()

# =============================================================================
# Copy necessary hoc files to build directory if they have not been copied yet
# =============================================================================
if(NOT TARGET hoc_module)
  add_custom_command(
    TARGET testneuron
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/share/lib
            ${PROJECT_BINARY_DIR}/share/nrn/lib)
endif()

# =============================================================================
# Use add_test method and set environment for testneuron (NEURONHOME env variable needs to be set to
# run testneuron and other tests based on nrniv)
# =============================================================================
foreach(test_name testneuron)
  add_test(NAME ${test_name} COMMAND ${test_name})
  list(APPEND TESTS ${test_name})
endforeach()

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
    COMMAND ${MPIEXEC} ${MPIEXEC_OVERSUBSCRIBE} --version
    RESULT_VARIABLE MPIEXEC_OVERSUBSCRIBE_TEST
    OUTPUT_QUIET ERROR_QUIET)
  if(NOT MPIEXEC_OVERSUBSCRIBE_TEST EQUAL 0)
    message(STATUS "mpiexec does not support ${MPIEXEC_OVERSUBSCRIBE}")
    unset(MPIEXEC_OVERSUBSCRIBE)
  endif()
endif()

include(NeuronTestHelper)
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
    set(python ${mpi_init_prefix${mpiexec}} ${PYTHON_EXECUTABLE} ${mpi_init_suffix${mpiexec}})
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
    COMMAND NEURON_INIT_MPI=1 ${python} -c "from neuron import h$<SEMICOLON> h.quit()")
endforeach()

# =============================================================================
# Add pytest
# =============================================================================
if(NRN_ENABLE_PYTHON AND PYTEST_FOUND)
  set(pytest -m pytest)
  if(NRN_SANITIZERS)
    # We configure the santiziers to abort on errors, which can lead to unhelpful output from
    # pytest. Passing -s means the sanitizer output will be visible in the log.
    list(APPEND pytest -s)
  endif()
  if(PYTEST_COV_FOUND)
    list(APPEND pytest --cov-report=xml --cov=neuron)
  endif()
  # TODO: consider allowing the group-related parts to be dropped here
  nrn_add_test_group(NAME pynrn MODFILE_PATTERNS test/pynrn/*.mod)
  nrn_add_test(
    GROUP pynrn
    NAME basic_tests
    PRELOAD_SANITIZER
    COMMAND COVERAGE_FILE=.coverage.basic_tests ${PYTHON_EXECUTABLE} ${pytest} ./test/pynrn
    SCRIPT_PATTERNS test/pynrn/*.py)

  # Mostly to increase coverage
  nrn_add_test_group(NAME coverage_tests MODFILE_PATTERNS test/cover/mod/*.mod)
  nrn_add_test(
    GROUP coverage_tests
    NAME cover_tests
    PRELOAD_SANITIZER
    COMMAND COVERAGE_FILE=.coverage.basic_tests ${PYTHON_EXECUTABLE} ${pytest} ./test/cover
    SCRIPT_PATTERNS test/cover/*.py test/cover/*.json)

  if(NRN_ENABLE_RX3D)
    nrn_add_test_group(
      NAME rxdmod_tests
      MODFILE_PATTERNS test/rxd/ecs/*.mod
      SCRIPT_PATTERNS test/rxd/*.py test/rxd/3d/*.asc test/rxd/testdata/*.dat)
    # These tests include comparisons against saved data, which appears to have been generated using
    # GCC. Other compilers produce larger differences.
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
      set(change_test_tolerance NRN_RXD_TEST_TOLERANCE=1e-8)
    elseif(NRN_HAVE_NVHPC_COMPILER)
      set(change_test_tolerance NRN_RXD_TEST_TOLERANCE=1e-4)
    endif()
    nrn_add_test(
      GROUP rxdmod_tests
      NAME rxd_tests
      PRELOAD_SANITIZER
      COMMAND COVERAGE_FILE=.coverage.rxd_tests ${change_test_tolerance} ${PYTHON_EXECUTABLE}
              ${pytest} ./test/rxd)
    if(NRN_ENABLE_MPI)
      nrn_find_python_module(mpi4py)
      if(mpi4py_FOUND)
        nrn_add_test(
          GROUP rxdmod_tests
          NAME rxd_mpi_tests
          PRELOAD_SANITIZER
          COMMAND
            COVERAGE_FILE=.coverage.rxd_mpi_tests ${change_test_tolerance} ${MPIEXEC_NAME}
            ${MPIEXEC_NUMPROC_FLAG} 1 ${MPIEXEC_PREFLAGS} ${PYTHON_EXECUTABLE} ${MPIEXEC_POSTFLAGS}
            ${pytest} ./test/rxd --mpi)
      endif()
    endif()
  endif()
  if(NRN_ENABLE_MPI)
    add_test(
      NAME parallel_tests
      COMMAND
        ${CMAKE_COMMAND} -E env ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS} ${CMAKE_BINARY_DIR}/bin/nrniv ${MPIEXEC_POSTFLAGS} -mpi -python
        ${PROJECT_SOURCE_DIR}/test/parallel_tests/test_subworld.py)
    add_test(
      NAME parallel_partrans
      COMMAND
        ${CMAKE_COMMAND} -E env ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS} ${CMAKE_BINARY_DIR}/bin/nrniv ${MPIEXEC_POSTFLAGS} -mpi -python
        ${PROJECT_SOURCE_DIR}/test/pynrn/test_partrans.py)
    list(APPEND TESTS parallel_tests parallel_partrans)
    add_test(
      NAME parallel_netpar
      COMMAND
        ${CMAKE_COMMAND} -E env ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS} ${CMAKE_BINARY_DIR}/bin/nrniv ${MPIEXEC_POSTFLAGS} -mpi -python
        ${PROJECT_SOURCE_DIR}/test/pynrn/test_netpar.py)
    list(APPEND TESTS parallel_tests parallel_netpar)
    add_test(
      NAME parallel_bas
      COMMAND
        ${CMAKE_COMMAND} -E env ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
        ${CMAKE_BINARY_DIR}/bin/nrniv ${MPIEXEC_POSTFLAGS} -mpi -python
        ${PROJECT_SOURCE_DIR}/test/parallel_tests/test_bas.py)
    list(APPEND TESTS parallel_tests parallel_bas)
  endif()
  # CoreNEURON's reports require MPI and segfault if it is not initialised. This is a crude
  # workaround.
  if(CORENRN_ENABLE_REPORTING)
    set(nrniv_mpi_arg -mpi)
    set(nrnpython_mpi_env NEURON_INIT_MPI=1)
    # see #1619, required when coreneuron is linked to sonata library
    set(sonata_zero_gid_env LIBSONATA_ZERO_BASED_GIDS=1)
  endif()
  # When GPU support is enabled then CoreNEURON is linked statically (libcorenrnmech.a) and cannot
  # be dynamically loaded by Python or nrniv. In that case (and only in that case, so we still test
  # the dynamic loading) then launch the tests with `special [-python]`. If you consider changing
  # how ${modtests_launch_py_mpi} is constructed (notably the use of ${MPIEXEC_NAME} instead of
  # ${MPIEXEC}) then first refer to GitHub issue #894 and note that nrn_add_test prefixes the
  # command with ${CMAKE_COMMAND} -E env.
  if(${CORENRN_ENABLE_GPU})
    # ${nrniv_mpi_arg} is not enough here, it crashes without -mpi even if CoreNEURON reports are
    # disabled.
    set(modtests_launch_hoc special -notatty -mpi)
    set(modtests_launch_py ${sonata_zero_gid_env} ${modtests_launch_hoc} -python -pyexe
                           ${PYTHON_EXECUTABLE})
    set(modtests_launch_py_mpi
        ${MPIEXEC_NAME}
        ${MPIEXEC_NUMPROC_FLAG}
        2
        ${MPIEXEC_PREFLAGS}
        special
        ${MPIEXEC_POSTFLAGS}
        -notatty
        -python
        -pyexe
        ${PYTHON_EXECUTABLE})
  else()
    set(modtests_preload_sanitizer PRELOAD_SANITIZER)
    set(modtests_launch_py ${sonata_zero_gid_env} ${nrnpython_mpi_env} ${PYTHON_EXECUTABLE}
                           ${pytest})
    set(modtests_launch_hoc ${CMAKE_BINARY_DIR}/bin/nrniv ${nrniv_mpi_arg})
    set(modtests_launch_py_mpi ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
                               ${PYTHON_EXECUTABLE})
  endif()

  # External coreneuron can be used for testing but for simplicity we are testing only submodule
  # builds (in near future we want to support only internal builds anyway)
  if(NOT nrn_using_ext_corenrn)
    nrn_add_test_group(
      CORENEURON
      NAME coreneuron_standalone
      MODFILE_PATTERNS NONE)

    nrn_add_test(
      GROUP coreneuron_standalone
      NAME test_nrn_corenrn_standalone
      REQUIRES coreneuron ${modtests_preload_sanitizer}
      # In a GPU build ${modtests_launch_py} needs to be a statically-linked `special`. As a
      # standard NEURON installation doesn't include `special` but only `nrniv` (without link to
      # `libcorenrnmech.a`), we can not run this test on GPUs.
      CONFLICTS gpu
      SCRIPT_PATTERNS test/coreneuron/test_psolve.py
      COMMAND COVERAGE_FILE=.coverage.coreneuron_standalone_test_psolve_py ${modtests_launch_py}
              test/coreneuron/test_psolve.py)
  endif()

  nrn_add_test_group(
    CORENEURON
    NAME coreneuron_modtests
    # This get used in 4 tests so make it the default and override in other tests.
    SCRIPT_PATTERNS test/coreneuron/test_spikes.py
    MODFILE_PATTERNS
      "test/coreneuron/mod files/*.mod" "test/coreneuron/mod files/axial.inc"
      test/pynrn/unitstest.mod test/pynrn/version_macros.mod test/gjtests/natrans.mod)
  nrn_add_test(
    GROUP coreneuron_modtests
    NAME version_macros
    REQUIRES coreneuron ${modtests_preload_sanitizer}
    SCRIPT_PATTERNS test/pynrn/test_version_macros.py
    COMMAND COVERAGE_FILE=.coverage.coreneuron_version_macros NRN_CORENEURON_ENABLE=true
            ${modtests_launch_py} test/pynrn/test_version_macros.py)
  # In GPU builds run all of the tests on both CPU and GPU
  set(coreneuron_modtests_gpu_env CORENRN_ENABLE_GPU=true)
  foreach(processor cpu gpu)
    set(processor_env ${coreneuron_modtests_${processor}_env})
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME fornetcon_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_fornetcon.py
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_fornetcon_py
              ${modtests_launch_py} test/coreneuron/test_fornetcon.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME direct_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_direct.py
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_direct_py ${modtests_launch_py}
              test/coreneuron/test_direct.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME direct_hoc_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_direct.hoc
      COMMAND ${processor_env} ${modtests_launch_hoc} test/coreneuron/test_direct.hoc)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME spikes_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_spikes_py ${modtests_launch_py}
              test/coreneuron/test_spikes.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME spikes_file_mode_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_spikes_file_mode_py
              NRN_TEST_SPIKES_FILE_MODE=1 ${modtests_launch_py} test/coreneuron/test_spikes.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME fast_imem_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/pynrn/test_fast_imem.py
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_fast_imem_py
              ${modtests_launch_py} test/pynrn/test_fast_imem.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME datareturn_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_datareturn.py
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_datareturn_py
              ${modtests_launch_py} test/coreneuron/test_datareturn.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_units_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_units.py
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_test_units_py
              ${modtests_launch_py} test/coreneuron/test_units.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_netmove_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_netmove.py
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_test_netmove_py
              ${modtests_launch_py} test/coreneuron/test_netmove.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_pointer_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_pointer.py
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_test_pointer_py
              ${modtests_launch_py} test/coreneuron/test_pointer.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_watchrange_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_watchrange.py
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_test_watchrange_py
              ${modtests_launch_py} test/coreneuron/test_watchrange.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_psolve_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_psolve.py
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_test_psolve_py
              ${modtests_launch_py} test/coreneuron/test_psolve.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_ba_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/coreneuron/test_ba.py
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_test_psolve_py
              ${modtests_launch_py} test/coreneuron/test_ba.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_natrans_py_${processor}
      REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
      SCRIPT_PATTERNS test/gjtests/test_natrans.py
      COMMAND ${processor_env} COVERAGE_FILE=.coverage.coreneuron_test_natrans_py
              ${modtests_launch_py} test/gjtests/test_natrans.py)
    if(NRN_ENABLE_MPI)
      nrn_find_python_module(mpi4py)
      if(mpi4py_FOUND)
        nrn_add_test(
          GROUP coreneuron_modtests
          NAME spikes_mpi_py_${processor}
          REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
          PROCESSORS 2
          COMMAND ${processor_env} NRN_TEST_SPIKES_MPI4PY=1 ${modtests_launch_py_mpi}
                  test/coreneuron/test_spikes.py)
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
            ${MPIEXEC_PREFLAGS}
            special
            ${MPIEXEC_POSTFLAGS}
            -python
            -pyexe
            ${PYTHON_EXECUTABLE})
        message(STATUS "Hitting workaround")
      else()
        set(launch_spikes_mpi_file_mode_py ${modtests_launch_py_mpi})
      endif()
      nrn_add_test(
        GROUP coreneuron_modtests
        NAME spikes_mpi_file_mode_py_${processor}
        REQUIRES coreneuron ${processor} ${modtests_preload_sanitizer}
        PROCESSORS 2
        COMMAND ${processor_env} NRN_TEST_SPIKES_NRNMPI_INIT=1 NRN_TEST_SPIKES_FILE_MODE=1
                ${launch_spikes_mpi_file_mode_py} test/coreneuron/test_spikes.py)
      nrn_add_test(
        GROUP coreneuron_modtests
        NAME inputpresyn_py_${processor}
        REQUIRES coreneuron ${processor}
        PROCESSORS 2
        COMMAND
          ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS} special
          ${MPIEXEC_POSTFLAGS} -mpi -python
          ${PROJECT_SOURCE_DIR}/test/coreneuron/test_inputpresyn.py)
    endif()
  endforeach()
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
add_modlunit_test("${PROJECT_SOURCE_DIR}/test/pynrn/unitstest.mod")
add_modlunit_test("${PROJECT_SOURCE_DIR}/src/nrnoc/hh.mod")
add_modlunit_test("${PROJECT_SOURCE_DIR}/src/nrnoc/stim.mod")
add_modlunit_test("${PROJECT_SOURCE_DIR}/src/nrnoc/pattern.mod")
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
