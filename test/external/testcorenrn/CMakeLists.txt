# Set up the tests from the testcorenrn external repository
include(NeuronTestHelper)
set(direct_tests netstimdirect)
set(spike_comparison_tests
    bbcore
    conc
    deriv
    gf
    kin
    patstim
    vecplay
    watch
    vecevent)

# Set the number of MPI ranks to use for each test. Assume 1 if not explicitly specified.
set(mpi_ranks_gf 2)
set(mpi_ranks_watch 2)
set(mpi_ranks_patstim 2)
set(mpi_ranks_vecplay 2)
set(mpi_ranks_vecevent 4)
set(mpi_ranks_netstimdirect 2)

# Flag which tests include `insert hh` and may, therefore, be sensitive to differences between
# NEURON and CoreNEURON when neither CoreNEURON nor modfile compatibility is enabled
set(test_gf_neuron_requirements mod_compatibility)
set(test_patstim_neuron_requirements mod_compatibility)
set(test_vecplay_neuron_requirements mod_compatibility)

get_filename_component(MPIEXEC_NAME ${MPIEXEC} NAME)

if(${CORENRN_ENABLE_GPU})
  set(gpu_args_online -c arg_coreneuron_gpu=1)
  set(gpu_args_offline --gpu)
endif()
# Run tests that require separate runs of NEURON and CoreNEURON to compare their spikes only
foreach(test ${spike_comparison_tests})
  if(DEFINED mpi_ranks_${test})
    set(num_ranks ${mpi_ranks_${test}})
  else()
    set(num_ranks 1)
  endif()
  nrn_add_test_group(
    NAME testcorenrn_${test}
    SUBMODULE tests/testcorenrn
    MODFILE_PATTERNS mod/*.mod
    SCRIPT_PATTERNS common.hoc defvar.hoc test${test}.hoc
    OUTPUT asciispikes::out${test}.dat)
  # Set up a test that runs the simulation using NEURON.
  nrn_add_test(
    GROUP testcorenrn_${test}
    NAME neuron
    REQUIRES mpi ${test_${test}_neuron_requirements}
    PROCESSORS ${num_ranks}
    COMMAND
      OMP_NUM_THREADS=1 HOC_LIBRARY_PATH=. ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${num_ranks}
      ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special ${MPIEXEC_POSTFLAGS} -mpi -c
      arg_tstop=100 test${test}.hoc)
  # Set up a test that runs the simulation using CoreNEURON's online mode
  if(NOT "${test}" STREQUAL "patstim")
    # Exclude the patstim test from running via CoreNEURON online mode
    nrn_add_test(
      GROUP testcorenrn_${test}
      NAME coreneuron_online
      REQUIRES coreneuron mpi python
      PROCESSORS ${num_ranks}
      COMMAND
        OMP_NUM_THREADS=1 HOC_LIBRARY_PATH=. ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${num_ranks}
        ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special ${MPIEXEC_POSTFLAGS} -mpi -c
        arg_tstop=100 -c arg_coreneuron=1 ${gpu_args_online} test${test}.hoc)
  endif()

  set(extra_args)
  if("${test}" STREQUAL "patstim")
    list(APPEND extra_args --pattern patstim.spk)
  endif()
  # Set up a test that runs the simulation using CoreNEURON's offline mode. First we must run NEURON
  # and tell it to dump the model without simulating it.
  nrn_add_test(
    GROUP testcorenrn_${test}
    NAME coreneuron_offline
    REQUIRES coreneuron mpi
    PROCESSORS ${num_ranks}
    PRECOMMAND OMP_NUM_THREADS=1
               HOC_LIBRARY_PATH=.
               ${MPIEXEC_NAME}
               ${MPIEXEC_NUMPROC_FLAG}
               ${num_ranks}
               ${MPIEXEC_OVERSUBSCRIBE}
               ${MPIEXEC_PREFLAGS}
               special
               -mpi
               -c
               arg_tstop=100
               -c
               arg_dump_coreneuron_model=1
               test${test}.hoc
    COMMAND
      OMP_NUM_THREADS=1 ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${num_ranks}
      ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special-core ${MPIEXEC_POSTFLAGS} -d coredat
      --mpi -e 100 ${extra_args} ${gpu_args_offline}
    OUTPUT asciispikes::out.dat)
  nrn_add_test_group_comparison(
    GROUP testcorenrn_${test}
    REFERENCE_OUTPUT asciispikes::external/tests/testcorenrn/reference/out_${test}.spk)
endforeach()

# Run the direct_tests that compare internally NEURON and CoreNEURON simulations
foreach(test ${direct_tests})
  if(DEFINED mpi_ranks_${test})
    set(num_ranks ${mpi_ranks_${test}})
  else()
    set(num_ranks 1)
  endif()
  nrn_add_test_group(
    NAME testcorenrn_${test}
    SUBMODULE tests/testcorenrn
    MODFILE_PATTERNS mod/*.mod
    SCRIPT_PATTERNS common.hoc defvar.hoc test${test}.hoc
    OUTPUT asciispikes::out${test}.dat)
  nrn_add_test(
    GROUP testcorenrn_${test}
    NAME direct
    REQUIRES coreneuron mpi python
    PROCESSORS ${num_ranks}
    COMMAND
      HOC_LIBRARY_PATH=. ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${num_ranks}
      ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special ${MPIEXEC_POSTFLAGS} -mpi -c
      arg_tstop=100 test${test}.hoc)
  nrn_add_test_group_comparison(
    GROUP testcorenrn_${test}
    REFERENCE_OUTPUT asciispikes::external/tests/testcorenrn/reference/out_${test}.spk)
endforeach()
