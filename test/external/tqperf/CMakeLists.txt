include(NeuronTestHelper)
nrn_add_test_group(
  NAME tqperf
  SUBMODULE tests/tqperf
  MODFILE_PATTERNS "mod/*.mod" "modx/*.mod"
  SCRIPT_PATTERNS "test1.py" "*.hoc")
set(tqperf_mpi_ranks 16)
nrn_add_test(
  GROUP tqperf
  NAME coreneuron
  REQUIRES mpi python coreneuron
  PROCESSORS ${tqperf_mpi_ranks}
  NRNIVMODL_ARGS -loadflags -lcrypto
  COMMAND
    ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${tqperf_mpi_ranks}
    ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special ${MPIEXEC_POSTFLAGS} -mpi -python test1.py)
