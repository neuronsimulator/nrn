include(NeuronTestHelper)
nrn_add_test_group(
  NAME reduced_dentate
  SUBMODULE tests/reduced_dentate
  OUTPUT "asciispikes::results/dentatenet_spikeout_*.dat"
  MODFILE_PATTERNS "mechanisms/*.mod"
  SCRIPT_PATTERNS "commonutils.py" "dat/*.dat" "datasets/Control/*.dat" "*.hoc" "templates/*.hoc")

set(reduced_dentate_mpi_ranks 4)
set(reduced_dentate_sim_time 10)
set(reduced_dentate_neuron_prefix
    OMP_NUM_THREADS=1
    HOC_LIBRARY_PATH=templates
    ${MPIEXEC_NAME}
    ${MPIEXEC_NUMPROC_FLAG}
    ${reduced_dentate_mpi_ranks}
    ${MPIEXEC_OVERSUBSCRIBE}
    ${MPIEXEC_PREFLAGS}
    special
    ${MPIEXEC_POSTFLAGS}
    -mpi
    -c
    mytstop=${reduced_dentate_sim_time}
    -c
    max_cells_per_type=100)
nrn_add_test(
  GROUP reduced_dentate
  NAME neuron
  REQUIRES mpi python
  PROCESSORS ${reduced_dentate_mpi_ranks}
  COMMAND ${reduced_dentate_neuron_prefix} run.hoc)
set(reduced_dentate_cpu_arg -c gpu=0)
set(reduced_dentate_gpu_arg -c gpu=1)
foreach(processor cpu gpu)
  nrn_add_test(
    GROUP reduced_dentate
    NAME coreneuron_${processor}
    REQUIRES coreneuron mpi ${processor} python
    PROCESSORS ${reduced_dentate_mpi_ranks}
    COMMAND ${reduced_dentate_neuron_prefix} -c coreneuron=1 ${reduced_dentate_${processor}_arg}
            run.hoc)
endforeach()
nrn_add_test_group_comparison(GROUP reduced_dentate)
