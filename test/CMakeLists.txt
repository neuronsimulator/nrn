set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(${PROJECT_SOURCE_DIR}/test ${PROJECT_SOURCE_DIR}/src/nrnoc
                    ${PROJECT_SOURCE_DIR}/src/nrniv ${PROJECT_SOURCE_DIR}/src/oc)

# =============================================================================
# Test executables
# =============================================================================
set(TEST_SOURCES unit_tests/oc/hoc_interpreter.cpp)
add_executable(testneuron unit_tests/unit_test.cpp ${TEST_SOURCES})
target_compile_features(testneuron PUBLIC cxx_std_11)
target_link_libraries(testneuron Catch2::Catch2 nrniv_lib ${CMAKE_THREAD_LIBS_INIT})
if(NOT MINGW)
  target_link_libraries(testneuron ${CMAKE_DL_LIBS})
endif()

# =============================================================================
# Copy necessary hoc files to build directory if they have not been copied yet
# =============================================================================
add_custom_command(
  TARGET testneuron POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/share/lib
          ${PROJECT_BINARY_DIR}/share/nrn/lib)

# Note that DYLD_LIBRARY_PATH is not required and interfere with dlopen
set(TEST_ENV NEURONHOME=${PROJECT_BINARY_DIR}/share/nrn NRNHOME=${PROJECT_BINARY_DIR}
             LD_LIBRARY_PATH=${PROJECT_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH})
if(NRN_ENABLE_PYTHON)
  list(
    APPEND
      TEST_ENV
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
  # TODO: consider allowing the group-related parts to be dropped here
  nrn_add_test_group(NAME pynrn_tests)
  nrn_add_test(
    GROUP pynrn_tests
    NAME basic_tests
    COMMAND
      COVERAGE_FILE=.coverage.basic_tests ${PYTHON_EXECUTABLE} -m pytest --cov-report=xml
      --cov=neuron ./test/pynrn
    MODFILE_PATTERNS test/pynrn/*.mod
    SCRIPT_PATTERNS test/pynrn/*.py)

  if(NRN_ENABLE_RX3D)
    nrn_add_test_group(
      NAME rxdmod_tests
      MODFILE_PATTERNS test/rxd/ecs/*.mod
      SCRIPT_PATTERNS test/rxd/*.py test/rxd/testdata/*.dat)
    nrn_add_test(
      GROUP rxdmod_tests
      NAME rxd_tests
      COMMAND
        COVERAGE_FILE=.coverage.rxd_tests ${PYTHON_EXECUTABLE} -m pytest --cov-report=xml
        --cov=neuron ./test/rxd)
    if(NRN_ENABLE_MPI)
      find_python_module(mpi4py)
      if(mpi4py_FOUND)
        get_filename_component(MPIEXEC_NAME ${MPIEXEC} NAME)
        nrn_add_test(
          GROUP rxdmod_tests
          NAME rxd_mpi_tests
          COMMAND
            COVERAGE_FILE=.coverage.rxd_mpi_tests ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 1
            ${MPIEXEC_PREFLAGS} ${PYTHON_EXECUTABLE} ${MPIEXEC_POSTFLAGS} -m pytest
            --cov-report=xml --cov=neuron ./test/rxd --mpi)
      endif()
    endif()
  endif()
  if(NRN_ENABLE_MPI)
    # Launching mpi executable with full path can mangle different python versions and libraries
    # (see issue #894). ${MPIEXEC_NAME} would reinsert the full path, but ${CMAKE_COMMAND} -E env
    # ${MPIEXEC_NAME} does not.
    get_filename_component(MPIEXEC_NAME ${MPIEXEC} NAME)
    add_test(
      NAME parallel_tests
      COMMAND
        ${CMAKE_COMMAND} -E env ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS} ${CMAKE_BINARY_DIR}/bin/nrniv ${MPIEXEC_POSTFLAGS} -mpi -python
        ${PROJECT_SOURCE_DIR}/test/parallel_tests/test_subworld.py)
    list(APPEND TESTS parallel_tests)
  endif()

  if(NRN_ENABLE_CORENEURON)
    # When GPU support is enabled then CoreNEURON is linked statically (libcorenrnmech.a) and cannot
    # be dynamically loaded by Python. This is why all the tests that rely on that dynamic loading
    # declare `CONFLICTS gpu`.
    nrn_add_test_group(
      NAME coreneuron_modtests
      SCRIPT_PATTERNS test/coreneuron/*.py
      MODFILE_PATTERNS test/coreneuron/mod/*.mod test/pynrn/unitstest.mod test/gjtests/natrans.mod)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME direct_py
      REQUIRES coreneuron
      CONFLICTS gpu
      COMMAND
        COVERAGE_FILE=.coverage.coreneuron_direct_py ${PYTHON_EXECUTABLE} -m pytest
        --cov-report=xml --cov=neuron test/coreneuron/test_direct.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME direct_hoc
      REQUIRES coreneuron
      SCRIPT_PATTERNS test/coreneuron/test_direct.hoc
      COMMAND ${CMAKE_BINARY_DIR}/bin/nrniv test/coreneuron/test_direct.hoc)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME spikes_py
      REQUIRES coreneuron
      CONFLICTS gpu
      COMMAND
        COVERAGE_FILE=.coverage.coreneuron_spikes_py ${PYTHON_EXECUTABLE} -m pytest
        --cov-report=xml --cov=neuron test/coreneuron/test_spikes.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME spikes_file_mode_py
      REQUIRES coreneuron
      CONFLICTS gpu
      COMMAND ${PYTHON_EXECUTABLE} test/coreneuron/test_spikes.py file_mode)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME fornetcon_py
      REQUIRES coreneuron
      CONFLICTS gpu
      COMMAND
        COVERAGE_FILE=.coverage.coreneuron_fornetcon_py ${PYTHON_EXECUTABLE} -m pytest
        --cov-report=xml --cov=neuron test/coreneuron/test_fornetcon.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME datareturn_py
      REQUIRES coreneuron
      CONFLICTS gpu
      COMMAND
        COVERAGE_FILE=.coverage.coreneuron_datareturn_py ${PYTHON_EXECUTABLE} -m pytest
        --cov-report=xml --cov=neuron test/coreneuron/test_datareturn.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_units_py
      REQUIRES coreneuron
      CONFLICTS gpu
      COMMAND
        COVERAGE_FILE=.coverage.coreneuron_test_units_py ${PYTHON_EXECUTABLE} -m pytest
        --cov-report=xml --cov=neuron test/coreneuron/test_units.py)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME test_natrans_py
      REQUIRES coreneuron
      CONFLICTS gpu
      SCRIPT_PATTERNS test/gjtests/test_natrans.py
      COMMAND
        COVERAGE_FILE=.coverage.coreneuron_test_natrans_py ${PYTHON_EXECUTABLE} -m pytest
        --cov-report=xml --cov=neuron test/gjtests/test_natrans.py)
    if(NRN_ENABLE_MPI)
      find_python_module(mpi4py)
      # Using -pyexe was a first workaround for GitHub issue #894, but it seems that we also need to
      # avoid using the full path to mpiexec (see other discussion in #894). Replacing ${MPIEXEC}
      # with ${CMAKE_COMMAND} -E env ${MPIEXEC_NAME} achieves this.
      get_filename_component(MPIEXEC_NAME ${MPIEXEC} NAME)
      if(mpi4py_FOUND)
        nrn_add_test(
          GROUP coreneuron_modtests
          NAME spikes_mpi_py
          REQUIRES coreneuron
          CONFLICTS gpu
          PROCESSORS 2
          COMMAND
            ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS} ${PYTHON_EXECUTABLE}
            ${MPIEXEC_POSTFLAGS} test/coreneuron/test_spikes.py mpi4py)
      endif()
      nrn_add_test(
        GROUP coreneuron_modtests
        NAME spikes_mpi_file_mode_py
        REQUIRES coreneuron
        PROCESSORS 2
        COMMAND
          ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS} special
          ${MPIEXEC_POSTFLAGS} -python -pyexe ${PYTHON_EXECUTABLE} test/coreneuron/test_spikes.py
          nrnmpi_init file_mode)
    endif()
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
  COMMAND
    ${CMAKE_COMMAND} -E env ${TEST_ENV} $ENV{SHELL} ${PROJECT_BINARY_DIR}/run_modlunit.bash
    \"${MODFILES_ACCEPT}\" \"${MODFILES_REJECT}\"
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  DEPENDS ${PROJECT_BINARY_DIR}/bin/modlunit)

# list(APPEND TESTS modlunit_test)

set_tests_properties(${TESTS} PROPERTIES ENVIRONMENT "${TEST_ENV}")

# Add tests that are configured using external repositories
add_subdirectory(external)
