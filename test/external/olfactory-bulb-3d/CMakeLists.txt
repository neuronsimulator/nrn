include(NeuronTestHelper)
set(prefix "sim") # everything is under this prefix in the repository
set(olfcatory_bulb_3d_sim_time 50)
set(olfcatory_bulb_3d_mpi_ranks 4)
set(olfcatory_bulb_3d_neuron_prefix
    OMP_NUM_THREADS=1
    ${MPIEXEC_NAME}
    ${MPIEXEC_NUMPROC_FLAG}
    ${olfcatory_bulb_3d_mpi_ranks}
    ${MPIEXEC_OVERSUBSCRIBE}
    ${MPIEXEC_PREFLAGS}
    special
    ${MPIEXEC_POSTFLAGS})

nrn_add_test_group(
  NAME olfactory-bulb-3d
  SUBMODULE tests/olfactory-bulb-3d
  OUTPUT asciispikes::olfactory_bulb.spikes.dat.000
  MODFILE_PATTERNS "${prefix}/*.mod"
  SCRIPT_PATTERNS "${prefix}/*.py" "${prefix}/*.txt" "${prefix}/*.hoc" "${prefix}/*.dic")
set(olfactory_bulb_3d_neuron_args -mpi -python "bulb3dtest.py")
nrn_add_test(
  GROUP olfactory-bulb-3d
  NAME neuron
  REQUIRES mpi python
  PROCESSORS ${olfcatory_bulb_3d_mpi_ranks}
             SIM_DIRECTORY
             "sim"
  PRECOMMAND sed -i "s/runsim.build_part_model(.*/runsim.build_part_model([5], [])/g" bulb3dtest.py
  COMMAND ${olfcatory_bulb_3d_neuron_prefix} ${olfactory_bulb_3d_neuron_args} --tstop
          ${olfcatory_bulb_3d_sim_time})
foreach(processor gpu cpu)
  foreach(mode online)
    if(${processor} STREQUAL "gpu")
      set(olfcatory_bulb_3d_gpu_args "--gpu")
    else()
      set(olfcatory_bulb_3d_gpu_args "")
    endif()
    if(${mode} STREQUAL "online")
      set(olfcatory_bulb_3d_filemode "")
    else()
      set(olfcatory_bulb_3d_filemode "--filemode")
    endif()
    nrn_add_test(
      GROUP olfactory-bulb-3d
      NAME coreneuron_${processor}_${mode}
      REQUIRES coreneuron python ${processor}
      PROCESSORS ${olfcatory_bulb_3d_mpi_ranks}
                 SIM_DIRECTORY
                 "sim"
      PRECOMMAND sed -i "s/runsim.build_part_model(.*/runsim.build_part_model([5], [])/g"
                 bulb3dtest.py
      COMMAND
        ${olfcatory_bulb_3d_neuron_prefix} ${olfactory_bulb_3d_neuron_args} --tstop
        ${olfcatory_bulb_3d_sim_time} --coreneuron ${olfcatory_bulb_3d_gpu_args}
        ${olfcatory_bulb_3d_filemode})
  endforeach()
endforeach()
nrn_add_test_group_comparison(GROUP olfactory-bulb-3d)
