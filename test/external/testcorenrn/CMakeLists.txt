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
    vecevent
    watch)

set(modfiles mod/Gfluct3.mod mod/hhderiv.mod mod/hhkin.mod mod/hhwatch.mod mod/nacum.mod
             mod/vecevent.mod)

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

if(${CORENRN_ENABLE_GPU})
  set(gpu_args_online -c arg_coreneuron_gpu=1)
  set(gpu_args_offline --gpu)
endif()

function(set_psolve_test_mode_suffix psolve_test_mode)
  if(${psolve_test_mode} EQUAL 3)
    set(suffix "_psolve_alternate")
  else()
    set(suffix "")
  endif()
  set(psolve_test_mode_suffix
      "${suffix}"
      PARENT_SCOPE)
endfunction()

# Libsonata-report automatically converts 1-based GIDs to 0-based by subtracting 1 from them. If
# there is already a GID 0 in the reports, libsonata-report throws. Since some of the NEURON
# integration tests create 0-based GID cells we set the environment variable
# LIBSONATA_ZERO_BASED_GIDS to 1 to avoid libsonata-reports's automatic conversion of 1-based GIDs
# to 0-based that would trigger the throw.
set(SONATA_GIDS_ZERO "LIBSONATA_ZERO_BASED_GIDS=1")

# Run tests that require separate runs of NEURON and CoreNEURON to compare their spikes only
foreach(test ${spike_comparison_tests})
  if(DEFINED mpi_ranks_${test})
    set(num_ranks ${mpi_ranks_${test}})
  else()
    set(num_ranks 1)
  endif()
  nrn_add_test_group(
    CORENEURON
    NAME testcorenrn_${test}
    SUBMODULE tests/testcorenrn
    MODFILE_PATTERNS ${modfiles}
    SCRIPT_PATTERNS common.hoc defvar.hoc test${test}.hoc
    OUTPUT asciispikes::out${test}.dat)
  # Set up a test that runs the simulation using NEURON.
  nrn_add_test(
    GROUP testcorenrn_${test}
    NAME neuron
    ENVIRONMENT OMP_NUM_THREADS=1 HOC_LIBRARY_PATH=.
    REQUIRES mpi ${test_${test}_neuron_requirements}
    PROCESSORS ${num_ranks}
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${num_ranks} ${MPIEXEC_OVERSUBSCRIBE}
            ${MPIEXEC_PREFLAGS} special ${MPIEXEC_POSTFLAGS} -mpi -c arg_tstop=100 test${test}.hoc)

  # Set up tests that run the simulation using CoreNEURON. In principle we run all combinations of
  # [online, offline]*[CPU, GPU], but some of these may be disabled.
  foreach(processor gpu cpu)
    foreach(psolve_test_mode 0 3)
      set_psolve_test_mode_suffix(${psolve_test_mode})
      # Set up a test that runs the simulation using CoreNEURON's online mode
      if(NOT "${test}" STREQUAL "patstim")
        # Exclude the patstim test from running via CoreNEURON online mode
        nrn_add_test(
          GROUP testcorenrn_${test}
          NAME coreneuron_${processor}_online${psolve_test_mode_suffix}
          REQUIRES coreneuron mpi python ${processor}
          CONFLICTS ${test_${test}_coreneuron_${processor}_conflicts}
          PROCESSORS ${num_ranks}
          ENVIRONMENT OMP_NUM_THREADS=1 HOC_LIBRARY_PATH=. ${SONATA_GIDS_ZERO}
          COMMAND
            ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${num_ranks} ${MPIEXEC_OVERSUBSCRIBE}
            ${MPIEXEC_PREFLAGS} special ${MPIEXEC_POSTFLAGS} -mpi -c arg_tstop=100 -c
            arg_coreneuron=1 -c arg_psolve_test_mode=${psolve_test_mode}
            ${${processor}_args_online} test${test}.hoc)
      endif()
    endforeach()
    # This is a workaround to reduce the number of MPI ranks in the GPU version of the patstim test
    # and, we hope, avoid https://github.com/BlueBrain/CoreNeuron/issues/563
    set(local_mpi_ranks ${num_ranks})
    set(extra_args)
    if("${test}" STREQUAL "patstim")
      list(APPEND extra_args --pattern patstim.spk)
      if("${processor}" STREQUAL "gpu")
        set(local_mpi_ranks 1)
      endif()
    endif()
    # Set up a test that runs the simulation using CoreNEURON's offline mode. First we must run
    # NEURON and tell it to dump the model without simulating it.
    if("${test}" STREQUAL "patstim")
      set(testcorenrn_patstim_script
          ${PROJECT_BINARY_DIR}/test/testcorenrn_${test}/coreneuron_${processor}_offline_saverestore/coreneuron_${processor}_offline_saverestore.sh
      )
      file(
        WRITE ${testcorenrn_patstim_script}
        "#!/bin/bash\n"
        "set -e\n"
        "OMP_NUM_THREADS=1 HOC_LIBRARY_PATH=. ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG}"
        " ${local_mpi_ranks} ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special"
        " ${MPIEXEC_POSTFLAGS} -mpi -c arg_tstop=100 -c arg_dump_coreneuron_model=1"
        " test${test}.hoc\n"
        "echo 1000 > patstim.1.spk\n"
        "sed -n 2,1001p patstim.spk >> patstim.1.spk\n"
        "echo 1000 > patstim.2.spk\n"
        "sed -n 1002,2001p patstim.spk >> patstim.2.spk\n"
        "OMP_NUM_THREADS=1 ${SONATA_GIDS_ZERO} ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${local_mpi_ranks}"
        " ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special-core ${MPIEXEC_POSTFLAGS}"
        " -d coredat --mpi -e 49 --pattern patstim.1.spk ${${processor}_args_offline}"
        " --checkpoint checkpoint -o part1\n"
        "OMP_NUM_THREADS=1 ${SONATA_GIDS_ZERO} ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${local_mpi_ranks}"
        " ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special-core ${MPIEXEC_POSTFLAGS}"
        " -d coredat --mpi -e 100 --pattern patstim.2.spk ${${processor}_args_offline}"
        " --restore checkpoint -o part2\n"
        "cat part1/out.dat part2/out.dat > out.dat\n")
      nrn_add_test(
        GROUP testcorenrn_${test}
        NAME coreneuron_${processor}_offline_saverestore
        REQUIRES coreneuron mpi ${processor}
        CONFLICTS mpi_dynamic
        PROCESSORS ${local_mpi_ranks}
        COMMAND bash coreneuron_${processor}_offline_saverestore.sh
        OUTPUT asciispikes::out.dat)
    endif()
    nrn_add_test(
      GROUP testcorenrn_${test}
      NAME coreneuron_${processor}_offline
      REQUIRES coreneuron mpi ${processor}
      CONFLICTS mpi_dynamic
      ENVIRONMENT OMP_NUM_THREADS=1 HOC_LIBRARY_PATH=. ${SONATA_GIDS_ZERO}
      PROCESSORS ${local_mpi_ranks}
      PRECOMMAND ${MPIEXEC_NAME}
                 ${MPIEXEC_NUMPROC_FLAG}
                 ${local_mpi_ranks}
                 ${MPIEXEC_OVERSUBSCRIBE}
                 ${MPIEXEC_PREFLAGS}
                 special
                 ${MPIEXEC_POSTFLAGS}
                 -mpi
                 -c
                 arg_tstop=100
                 -c
                 arg_dump_coreneuron_model=1
                 test${test}.hoc
      COMMAND
        ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${local_mpi_ranks} ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS} special-core ${MPIEXEC_POSTFLAGS} -d coredat --mpi -e 100 ${extra_args}
        ${${processor}_args_offline}
      OUTPUT asciispikes::out.dat)
  endforeach()
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
    CORENEURON
    NAME testcorenrn_${test}
    SUBMODULE tests/testcorenrn
    MODFILE_PATTERNS ${modfiles}
    SCRIPT_PATTERNS common.hoc defvar.hoc test${test}.hoc
    OUTPUT asciispikes::out${test}.dat)
  foreach(psolve_test_mode 0 3)
    set_psolve_test_mode_suffix(${psolve_test_mode})
    nrn_add_test(
      GROUP testcorenrn_${test}
      NAME direct_${test}${psolve_test_mode_suffix}
      REQUIRES coreneuron mpi python
      PROCESSORS ${num_ranks}
      ENVIRONMENT HOC_LIBRARY_PATH=.
      COMMAND
        ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${num_ranks} ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS} special ${MPIEXEC_POSTFLAGS} -mpi -c arg_tstop=100 -c
        arg_psolve_test_mode=${psolve_test_mode} test${test}.hoc)
  endforeach()
  nrn_add_test_group_comparison(
    GROUP testcorenrn_${test}
    REFERENCE_OUTPUT asciispikes::external/tests/testcorenrn/reference/out_${test}.spk)
endforeach()
