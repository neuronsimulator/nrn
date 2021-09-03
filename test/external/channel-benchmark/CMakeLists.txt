include(NeuronTestHelper)
set(prefix "benchmark/channels") # everything is under this prefix in the repository
set(channel_benchmark_sim_time 10)
set(channel_benchmark_mpi_ranks 1)
set(channel_benchmark_neuron_prefix
    OMP_NUM_THREADS=1
    "HOC_LIBRARY_PATH=${prefix}/lib/hoclib"
    ${MPIEXEC_NAME}
    ${MPIEXEC_NUMPROC_FLAG}
    ${channel_benchmark_mpi_ranks}
    ${MPIEXEC_OVERSUBSCRIBE}
    ${MPIEXEC_PREFLAGS}
    special
    ${MPIEXEC_POSTFLAGS})

set(channel_benchmark_hippo_num_cells 6) # Should be at least 6 to cover all cell types
set(channel_benchmark_sscx_num_cells 16) # Similarly, should be at least 16.
set(channel_benchmark_gpu_args -c arg_coreneuron_gpu=1)
set(channel_benchmark_coreneuron_gpu_args --gpu --cell-permute=2)
foreach(model hippo sscx)
  nrn_add_test_group(
    NAME channel_benchmark_${model}
    SUBMODULE tests/channel-benchmark
    OUTPUT asciispikes::out.dat
    MODFILE_PATTERNS "${prefix}/lib/modlib/*.mod"
    SCRIPT_PATTERNS "${prefix}/lib/hoclib/*.hoc" "${prefix}/${model}/*.txt"
                    "${prefix}/${model}/morphologies/*.asc")
  set(channel_benchmark_neuron_args
      -mpi
      -c
      "strdef arg_datapath"
      -c
      "arg_datapath = \"${prefix}\""
      -c
      arg_target_count=${channel_benchmark_${model}_num_cells}
      -c
      arg_tstop=${channel_benchmark_sim_time}
      -c
      "strdef arg_model"
      -c
      "arg_model=\"${model}\""
      "${prefix}/lib/hoclib/init.hoc")
  nrn_add_test(
    GROUP channel_benchmark_${model}
    NAME neuron
    REQUIRES mpi
    PROCESSORS ${channel_benchmark_mpi_ranks}
    COMMAND ${channel_benchmark_neuron_prefix} ${channel_benchmark_neuron_args})
  foreach(processor gpu cpu)
    nrn_add_test(
      GROUP channel_benchmark_${model}
      NAME coreneuron_${processor}
      REQUIRES coreneuron ${processor}
      PROCESSORS ${channel_benchmark_mpi_ranks}
      COMMAND ${channel_benchmark_neuron_prefix} -c arg_coreneuron=1
              ${channel_benchmark_${processor}_args} ${channel_benchmark_neuron_args})
    nrn_add_test(
      GROUP channel_benchmark_${model}
      NAME coreneuron_${processor}_offline
      REQUIRES coreneuron mpi ${processor}
      PROCESSORS ${channel_benchmark_mpi_ranks}
      PRECOMMAND ${channel_benchmark_neuron_prefix} -c arg_dump_coreneuron_model=1
                 ${channel_benchmark_${processor}_args} ${channel_benchmark_neuron_args}
      COMMAND
        OMP_NUM_THREADS=1 ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${channel_benchmark_mpi_ranks}
        ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special-core ${MPIEXEC_POSTFLAGS} --mpi
        --tstop ${channel_benchmark_sim_time} --voltage 1000. -d coredat/
        ${channel_benchmark_coreneuron_${processor}_args})
  endforeach()
  nrn_add_test_group_comparison(GROUP channel_benchmark_${model})
endforeach()
