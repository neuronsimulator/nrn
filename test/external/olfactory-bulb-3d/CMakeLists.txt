include(NeuronTestHelper)
set(olfactory_bulb_3d_sim_time 50)
set(olfactory_bulb_3d_mpi_ranks 4)
if(NRN_ENABLE_CORENEURON AND NOT CORENRN_ENABLE_SHARED)
  set(launch_executable special)
  set(olfactory_bulb_3d_neuron_args -mpi -python)
else()
  set(preload_sanitizer PRELOAD_SANITIZER)
  set(launch_executable ${preload_sanitizer_mpiexec} ${NRN_DEFAULT_PYTHON_EXECUTABLE})
  set(extra_env NEURON_INIT_MPI=1)
endif()
list(APPEND olfactory_bulb_3d_neuron_args bulb3dtest.py)
set(olfactory_bulb_3d_neuron_prefix
    ${MPIEXEC_NAME}
    ${MPIEXEC_NUMPROC_FLAG}
    ${olfactory_bulb_3d_mpi_ranks}
    ${MPIEXEC_OVERSUBSCRIBE}
    ${MPIEXEC_PREFLAGS}
    ${launch_executable}
    ${MPIEXEC_POSTFLAGS})

nrn_add_test_group(
  CORENEURON
  NAME olfactory-bulb-3d
  SUBMODULE tests/olfactory-bulb-3d
  ENVIRONMENT OMP_NUM_THREADS=1 ${extra_env}
  OUTPUT asciispikes::olfactory_bulb.spikes.dat.000
  MODFILE_PATTERNS "*.mod"
  SCRIPT_PATTERNS "*.py" "*.txt" "*.hoc" "*.dic"
  SIM_DIRECTORY sim)
# Note that the bulb3dtest.py gives an error if run without MPI
nrn_add_test(
  GROUP olfactory-bulb-3d
  NAME neuron ${preload_sanitizer}
  REQUIRES mpi python
  PROCESSORS ${olfactory_bulb_3d_mpi_ranks}
  PRECOMMAND sed -i.bak "s/runsim.build_part_model(.*/runsim.build_part_model([5], [])/g"
             bulb3dtest.py
  COMMAND ${olfactory_bulb_3d_neuron_prefix} ${olfactory_bulb_3d_neuron_args} --tstop
          ${olfactory_bulb_3d_sim_time})
foreach(processor gpu cpu)
  foreach(mode online)
    if(${processor} STREQUAL "gpu")
      set(olfactory_bulb_3d_gpu_args "--gpu")
    else()
      set(olfactory_bulb_3d_gpu_args "")
    endif()
    if(${mode} STREQUAL "online")
      set(olfactory_bulb_3d_filemode "")
    else()
      set(olfactory_bulb_3d_filemode "--filemode")
    endif()
    nrn_add_test(
      GROUP olfactory-bulb-3d
      NAME coreneuron_${processor}_${mode} ${preload_sanitizer}
      REQUIRES coreneuron mpi python ${processor}
      PROCESSORS ${olfactory_bulb_3d_mpi_ranks}
      PRECOMMAND sed -i.bak "s/runsim.build_part_model(.*/runsim.build_part_model([5], [])/g"
                 bulb3dtest.py
      COMMAND
        ${olfactory_bulb_3d_neuron_prefix} ${olfactory_bulb_3d_neuron_args} --tstop
        ${olfactory_bulb_3d_sim_time} --coreneuron ${olfactory_bulb_3d_gpu_args}
        ${olfactory_bulb_3d_filemode})
  endforeach()
endforeach()
nrn_add_test_group_comparison(GROUP olfactory-bulb-3d)
