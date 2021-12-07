include(NeuronTestHelper)
nrn_add_test_group(
  NAME tqperf
  SUBMODULE tests/tqperf
  SCRIPT_PATTERNS "*.py" "*.hoc" "ci-test.sh" "mod/*.mod" "modx/*.mod")

set(tqperf_mpi_ranks 16)
set(tqperf_sim_time 100)
set(tqperf_neuron_prefix
    OMP_NUM_THREADS=1
    ${MPIEXEC_NAME}
    ${MPIEXEC_NUMPROC_FLAG}
    ${tqperf_mpi_ranks}
    ${MPIEXEC_OVERSUBSCRIBE}
    ${MPIEXEC_PREFLAGS}
    special
    ${MPIEXEC_POSTFLAGS}
    -mpi
    -python
    )
nrn_add_test(
  GROUP tqperf
  NAME neuron
  REQUIRES mpi python coreneuron
  PROCESSORS ${tqperf_mpi_ranks}
  COMMAND bash "./ci-test.sh"
)
