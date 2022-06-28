include(NeuronTestHelper)
set(prefix "benchmark/channels") # everything is under this prefix in the repository
set(channel_benchmark_sim_time 10)
set(channel_benchmark_mpi_ranks 1)
set(channel_benchmark_neuron_prefix
    ${MPIEXEC_NAME}
    ${MPIEXEC_NUMPROC_FLAG}
    ${channel_benchmark_mpi_ranks}
    ${MPIEXEC_OVERSUBSCRIBE}
    ${MPIEXEC_PREFLAGS}
    special
    ${MPIEXEC_POSTFLAGS})

set(channel_benchmark_hippo_num_cells 6) # Should be at least 6 to cover all cell types
set(channel_benchmark_sscx_num_cells 16) # Similarly, should be at least 16.
set(channel_benchmark_gpu_args -c arg_coreneuron_gpu=1) # TODO: test --cell-permute=2
set(channel_benchmark_filemode_args -c arg_coreneuron_filemode=1)
foreach(model hippo sscx)
  nrn_add_test_group(
    CORENEURON
    NAME channel_benchmark_${model}
    SUBMODULE tests/channel-benchmark
    OUTPUT asciispikes::out.dat
    ENVIRONMENT OMP_NUM_THREADS=1 "HOC_LIBRARY_PATH=${prefix}/lib/hoclib"
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
    foreach(mode online filemode)
      nrn_add_test(
        GROUP channel_benchmark_${model}
        NAME coreneuron_${processor}_${mode}
        REQUIRES coreneuron ${processor}
        PROCESSORS ${channel_benchmark_mpi_ranks}
        COMMAND
          ${channel_benchmark_neuron_prefix} -c arg_coreneuron=1 ${channel_benchmark_${mode}_args}
          ${channel_benchmark_${processor}_args} ${channel_benchmark_neuron_args})
    endforeach()
  endforeach()
  nrn_add_test_group_comparison(GROUP channel_benchmark_${model})
endforeach()
