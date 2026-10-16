include(NeuronTestHelper)
nrn_add_test_group(
  CORENEURON
  NAME tqperf
  SUBMODULE tests/tqperf
  MODFILE_PATTERNS "mod/*.mod" "modx/*.mod"
  NRNIVMODL_ARGS -incflags "-I${OPENSSL_INCLUDE_DIR}" -loadflags "${OPENSSL_CRYPTO_LIBRARY}"
  SCRIPT_PATTERNS "test1.py" "*.hoc")
if("tqperf-heavy" IN_LIST NRN_ENABLE_MODEL_TESTS)
  set(tqperf_mpi_ranks 16)
else()
  set(tqperf_mpi_ranks 2)
endif()
set(python_executable ${preload_sanitizer_mpiexec} ${NRN_DEFAULT_PYTHON_EXECUTABLE})
set(special_executable special)
set(python_requires coreneuron_shared)
set(special_requires coreneuron)
set(special_args -mpi -python)
set(python_environment NEURON_INIT_MPI=1)
set(python_preload_sanitizer PRELOAD_SANITIZER)
foreach(binary special python)
  nrn_add_test(
    GROUP tqperf
    NAME coreneuron_${binary} ${${binary}_preload_sanitizer}
    REQUIRES mpi python ${${binary}_requires}
    PROCESSORS ${tqperf_mpi_ranks}
    ENVIRONMENT ${${binary}_environment}
    COMMAND
      ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${tqperf_mpi_ranks} ${MPIEXEC_OVERSUBSCRIBE}
      ${MPIEXEC_PREFLAGS} ${${binary}_executable} ${MPIEXEC_POSTFLAGS} ${${binary}_args} test1.py)
endforeach()
