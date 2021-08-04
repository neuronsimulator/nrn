set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(${PROJECT_SOURCE_DIR}/test ${PROJECT_SOURCE_DIR}/src/nrnoc
                    ${PROJECT_SOURCE_DIR}/src/nrniv ${PROJECT_SOURCE_DIR}/src/oc)

# =============================================================================
# Test executables
# =============================================================================
set(TEST_SOURCES unit_tests/oc/hoc_interpreter.cpp)
add_executable(testneuron unit_tests/unit_test.cpp ${TEST_SOURCES})
target_compile_features(testneuron PUBLIC cxx_std_11)
target_link_libraries(testneuron Catch2::Catch2 nrniv_lib)
if(${USE_PTHREAD} EQUAL 1)
  target_link_libraries(testneuron Threads::Threads)
endif()
if(NOT MINGW)
  target_link_libraries(testneuron ${CMAKE_DL_LIBS})
endif()

# =============================================================================
# Copy necessary hoc files to build directory if they have not been copied yet
# =============================================================================
add_custom_command(
  TARGET testneuron
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/share/lib
          ${PROJECT_BINARY_DIR}/share/nrn/lib)

# Note that DYLD_LIBRARY_PATH is not required and interfere with dlopen
set(TEST_ENV NEURONHOME=${PROJECT_BINARY_DIR}/share/nrn NRNHOME=${PROJECT_BINARY_DIR}
             LD_LIBRARY_PATH=${PROJECT_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH})
if(NOT coreneuron_FOUND)
  # CoreNEURON is being built internally
  list(APPEND TEST_ENV "CNRNHOME=${PROJECT_BINARY_DIR}")
endif()
if(NRN_ENABLE_PYTHON)
  list(APPEND TEST_ENV
       PYTHONPATH=${PROJECT_BINARY_DIR}/lib/python:${PROJECT_SOURCE_DIR}/test/rxd:$ENV{PYTHONPATH})
endif()
# Give the environment string a more descriptive name that can be used in NeuronTestHelper.cmake
set(NRN_TEST_ENV "${TEST_ENV}")

set(TESTS "")

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

# =============================================================================
# Add pytest
# =============================================================================
if(NRN_ENABLE_PYTHON AND PYTEST_FOUND)
  include(NeuronTestHelper)
  if(PYTEST_COV_FOUND)
    set(PYTEST_COVERAGE_ARGS --cov-report=xml --cov=neuron)
  else()
    unset(PYTEST_COVERAGE_ARGS)
  endif()
  # TODO: consider allowing the group-related parts to be dropped here
  nrn_add_test_group(NAME pynrn_tests)
  nrn_add_test(
    GROUP pynrn_tests
    NAME basic_tests
    COMMAND COVERAGE_FILE=.coverage.basic_tests ${PYTHON_EXECUTABLE} -m pytest
            ${PYTEST_COVERAGE_ARGS} ./test/pynrn
    MODFILE_PATTERNS test/pynrn/*.mod
    SCRIPT_PATTERNS test/pynrn/*.py)

  if(NRN_ENABLE_RX3D)
    nrn_add_test_group(
      NAME rxdmod_tests
      MODFILE_PATTERNS test/rxd/ecs/*.mod
      SCRIPT_PATTERNS test/rxd/*.py test/rxd/3d/*.asc test/rxd/testdata/*.dat)
    # These tests include comparisons against saved data, which appears to have been generated using
    # GCC. Other compilers produce larger differences.
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
      set(change_test_tolerance NRN_RXD_TEST_TOLERANCE=1e-8)
    elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "PGI" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "NVHPC")
      set(change_test_tolerance NRN_RXD_TEST_TOLERANCE=1e-4)
    endif()
    nrn_add_test(
      GROUP rxdmod_tests
      NAME rxd_tests
      COMMAND COVERAGE_FILE=.coverage.rxd_tests ${change_test_tolerance} ${PYTHON_EXECUTABLE} -m
              pytest ${PYTEST_COVERAGE_ARGS} ./test/rxd)
    if(NRN_ENABLE_MPI)
      find_python_module(mpi4py)
      if(mpi4py_FOUND)
        nrn_add_test(
          GROUP rxdmod_tests
          NAME rxd_mpi_tests
          COMMAND
            COVERAGE_FILE=.coverage.rxd_mpi_tests ${change_test_tolerance} ${MPIEXEC_NAME}
            ${MPIEXEC_NUMPROC_FLAG} 1 ${MPIEXEC_PREFLAGS} ${PYTHON_EXECUTABLE} ${MPIEXEC_POSTFLAGS}
            -m pytest ${PYTEST_COVERAGE_ARGS} ./test/rxd --mpi)
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
  endif()
  # CoreNEURON's reports require MPI and segfault if it is not initialised. This is a crude
  # workaround.
  if(CORENRN_ENABLE_REPORTING)
    set(nrniv_mpi_arg -mpi)
    set(nrnpython_mpi_env NEURON_INIT_MPI=1)
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
    set(modtests_launch_hoc CORENRN_ENABLE_GPU=true special -mpi)
    set(modtests_launch_py ${modtests_launch_hoc} -python -pyexe ${PYTHON_EXECUTABLE})
    set(modtests_launch_py_mpi
        CORENRN_ENABLE_GPU=true
        ${MPIEXEC_NAME}
        ${MPIEXEC_NUMPROC_FLAG}
        2
        ${MPIEXEC_PREFLAGS}
        special
        ${MPIEXEC_POSTFLAGS}
        -python
        -pyexe
        ${PYTHON_EXECUTABLE})
  else()
    set(py_args -m pytest ${PYTEST_COVERAGE_ARGS} -s)
    set(modtests_launch_py ${nrnpython_mpi_env} ${PYTHON_EXECUTABLE} ${py_args})
    set(modtests_launch_hoc ${CMAKE_BINARY_DIR}/bin/nrniv ${nrniv_mpi_arg})
    set(modtests_launch_py_mpi ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
                               ${PYTHON_EXECUTABLE})
  endif()
  nrn_add_test_group(
    NAME coreneuron_modtests
    # This get used in 4 tests so make it the default and override in other tests.
    SCRIPT_PATTERNS test/coreneuron/test_spikes.py
    MODFILE_PATTERNS test/coreneuron/mod/*.mod test/pynrn/unitstest.mod test/gjtests/natrans.mod)
  nrn_add_test(
    GROUP coreneuron_modtests
    NAME direct_py
    REQUIRES coreneuron
    SCRIPT_PATTERNS test/coreneuron/test_direct.py
    COMMAND COVERAGE_FILE=.coverage.coreneuron_direct_py ${modtests_launch_py}
            test/coreneuron/test_direct.py)
  nrn_add_test(
    GROUP coreneuron_modtests
    NAME direct_hoc
    REQUIRES coreneuron
    SCRIPT_PATTERNS test/coreneuron/test_direct.hoc
    COMMAND ${modtests_launch_hoc} test/coreneuron/test_direct.hoc)
  nrn_add_test(
    GROUP coreneuron_modtests
    NAME spikes_py
    REQUIRES coreneuron
    COMMAND COVERAGE_FILE=.coverage.coreneuron_spikes_py ${modtests_launch_py}
            test/coreneuron/test_spikes.py)
  nrn_add_test(
    GROUP coreneuron_modtests
    NAME spikes_file_mode_py
    REQUIRES coreneuron
    COMMAND COVERAGE_FILE=.coverage.coreneuron_spikes_file_mode_py NRN_TEST_SPIKES_FILE_MODE=1
            ${modtests_launch_py} test/coreneuron/test_spikes.py)
  # See https://github.com/BlueBrain/CoreNeuron/issues/512, this doesn't work on GPU.
  nrn_add_test(
    GROUP coreneuron_modtests
    NAME fornetcon_py
    REQUIRES coreneuron
    CONFLICTS gpu
    SCRIPT_PATTERNS test/coreneuron/test_fornetcon.py
    COMMAND COVERAGE_FILE=.coverage.coreneuron_fornetcon_py ${modtests_launch_py}
            test/coreneuron/test_fornetcon.py)
  nrn_add_test(
    GROUP coreneuron_modtests
    NAME datareturn_py
    REQUIRES coreneuron
    SCRIPT_PATTERNS test/coreneuron/test_datareturn.py
    COMMAND COVERAGE_FILE=.coverage.coreneuron_datareturn_py ${modtests_launch_py}
            test/coreneuron/test_datareturn.py)
  nrn_add_test(
    GROUP coreneuron_modtests
    NAME test_units_py
    REQUIRES coreneuron
    SCRIPT_PATTERNS test/coreneuron/test_units.py
    COMMAND COVERAGE_FILE=.coverage.coreneuron_test_units_py ${modtests_launch_py}
            test/coreneuron/test_units.py)
  nrn_add_test(
    GROUP coreneuron_modtests
    NAME test_natrans_py
    REQUIRES coreneuron
    SCRIPT_PATTERNS test/gjtests/test_natrans.py
    COMMAND COVERAGE_FILE=.coverage.coreneuron_test_natrans_py ${modtests_launch_py}
            test/gjtests/test_natrans.py)
  if(NRN_ENABLE_MPI)
    find_python_module(mpi4py)
    if(mpi4py_FOUND)
      nrn_add_test(
        GROUP coreneuron_modtests
        NAME spikes_mpi_py
        REQUIRES coreneuron
        PROCESSORS 2
        COMMAND NRN_TEST_SPIKES_MPI4PY=1 ${modtests_launch_py_mpi} test/coreneuron/test_spikes.py)
    endif()
    # See https://www.neuron.yale.edu/phpBB/viewtopic.php?t=4297 and links therein for more
    # discussion. Launching this test via `python` (not `special`) does not work on Linux with
    # OpenMPI 2.x and without dynamic MPI enabled in NEURON, which corresponds to the GitHub Actions
    # environment. GPU builds are already launched using `special`.
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
      NAME spikes_mpi_file_mode_py
      REQUIRES coreneuron
      PROCESSORS 2
      COMMAND NRN_TEST_SPIKES_NRNMPI_INIT=1 NRN_TEST_SPIKES_FILE_MODE=1
              ${launch_spikes_mpi_file_mode_py} test/coreneuron/test_spikes.py)
  endif()
endif()

# ============================================
# Test modlunit
# ============================================

# Strategy. Create a bash script that runs modlunit on a list of mod file expecting success and a
# list expecting failure. Then run the script with a custom target.

file(
  WRITE ${PROJECT_BINARY_DIR}/run_modlunit.bash
  "\
#!bash\n\
set -e\n\
M=${PROJECT_BINARY_DIR}/bin/modlunit
for i in $1 ; do\n\
  echo \"expect accept $i\"\n\
  $M $i\n\
done\n\
for i in $2 ; do\n\
  echo \"expect reject $i\"\n\
  if $M $i ; then\n\
    exit 1\n\
  fi\n\
done\n\
exit 0\n\
")

set(MODFILES_ACCEPT ${PROJECT_SOURCE_DIR}/test/pynrn/unitstest.mod
                    ${PROJECT_SOURCE_DIR}/src/nrnoc/hh.mod ${PROJECT_SOURCE_DIR}/src/nrnoc/stim.mod)
set(MODFILES_REJECT ${PROJECT_SOURCE_DIR}/src/nrnoc/pattern.mod)

add_custom_target(
  modlunit_test ALL
  COMMAND ${CMAKE_COMMAND} -E env ${TEST_ENV} $ENV{SHELL} ${PROJECT_BINARY_DIR}/run_modlunit.bash
          \"${MODFILES_ACCEPT}\" \"${MODFILES_REJECT}\"
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  DEPENDS ${PROJECT_BINARY_DIR}/bin/modlunit)

# list(APPEND TESTS modlunit_test)

set_tests_properties(${TESTS} PROPERTIES ENVIRONMENT "${TEST_ENV}")

# Add tests that are configured using external repositories
add_subdirectory(external)
