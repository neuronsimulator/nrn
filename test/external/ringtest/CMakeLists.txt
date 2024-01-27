# Set up tests from the ringtest external repository
include(NeuronTestHelper)
# Step 1 -- define a group of configurations
nrn_add_test_group(
  CORENEURON
  NAME external_ringtest
  SUBMODULE tests/ringtest # git submodule where the relevant tests are defined
  ENVIRONMENT OMP_NUM_THREADS=1
  MODFILE_PATTERNS "mod/*.mod"
  OUTPUT asciispikes::spk2.std # which output data to compare
  SCRIPT_PATTERNS "*.py" "*.hoc")

set(ringtest_mpi_ranks 2)
set(ringtest_prefix ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${ringtest_mpi_ranks}
                    ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS})
set(ringtest_special ${ringtest_prefix} special ${MPIEXEC_POSTFLAGS} -mpi -python ringtest.py)
set(ringtest_special_core ${ringtest_prefix} special-core ${MPIEXEC_POSTFLAGS})
set(ringtest_python ${ringtest_prefix} ${preload_sanitizer_mpiexec}
                    ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${MPIEXEC_POSTFLAGS} ringtest.py)

# Step 2 -- add configurations to the group (e.g. here NEURON without MPI) When CoreNEURON is
# enabled then TABLE statements are disabled in hh.mod, which causes slight numerical differences in
# the results from both NEURON and CoreNEURON. Consequently, if neither CoreNEURON nor
# NRN_ENABLE_MOD_COMPATIBILITY is enabled then NEURON does not match the reference file. To work
# around this only run the NEURON tests if CoreNEURON-compatible modfiles are used.
nrn_add_test(
  GROUP external_ringtest
  NAME neuron
  REQUIRES mod_compatibility python
  COMMAND special -python ringtest.py -tstop 100
  OUTPUT asciispikes::spk1.std)

nrn_add_test(
  GROUP external_ringtest
  NAME neuron_mpi
  REQUIRES mod_compatibility mpi python
  PROCESSORS ${ringtest_mpi_ranks}
  COMMAND ${ringtest_special} -tstop 100)

nrn_add_test(
  GROUP external_ringtest
  NAME neuron_mpi_python
  PRELOAD_SANITIZER
  REQUIRES mod_compatibility mpi python
  PROCESSORS ${ringtest_mpi_ranks}
  ENVIRONMENT NEURON_INIT_MPI=1
  COMMAND ${ringtest_python} -tstop 100)

foreach(processor cpu gpu)
  if("${processor}" STREQUAL "gpu")
    set(gpu_arg -gpu)
    set(special_gpu_arg --gpu)
  else()
    set(gpu_arg)
    set(special_gpu_arg)
  endif()
  set(ringtest_corenrn_script
      ${PROJECT_BINARY_DIR}/test/external_ringtest/coreneuron_${processor}_mpi_offline_saverestore/coreneuron_${processor}_mpi_offline_saverestore.sh
  )
  string(REPLACE ";" " " ringtest_special_str "${ringtest_special}")
  string(REPLACE ";" " " ringtest_special_core_str "${ringtest_special_core}")
  file(
    WRITE ${ringtest_corenrn_script}
    "#!/bin/bash\n"
    "set -e\n"
    "OMP_NUM_THREADS=1 ${ringtest_special_str} -tstop 0 -coreneuron -dumpmodel\n"
    "OMP_NUM_THREADS=1 LIBSONATA_ZERO_BASED_GIDS=1 ${ringtest_special_core_str} --mpi -d coredat/ -e 10 --checkpoint part0"
    " --outpath part0\n"
    "OMP_NUM_THREADS=1 LIBSONATA_ZERO_BASED_GIDS=1 ${ringtest_special_core_str} --mpi -d coredat/ -e 40 --checkpoint part1"
    " --restore part0 --outpath part1\n"
    "OMP_NUM_THREADS=1 LIBSONATA_ZERO_BASED_GIDS=1 ${ringtest_special_core_str} --mpi -d coredat/ -e 100 --checkpoint part2"
    " --restore part1 --outpath part2\n"
    "cat part0/out.dat > out.dat\n"
    "cat part1/out.dat >> out.dat\n"
    "cat part2/out.dat >> out.dat\n")
  nrn_add_test(
    GROUP external_ringtest
    NAME coreneuron_${processor}_mpi_offline_saverestore
    REQUIRES coreneuron mpi python ${processor}
    CONFLICTS mpi_dynamic
    PROCESSORS ${ringtest_mpi_ranks}
    COMMAND bash coreneuron_${processor}_mpi_offline_saverestore.sh
    OUTPUT asciispikes::out.dat)
  nrn_add_test(
    GROUP external_ringtest
    NAME coreneuron_${processor}_mpi
    REQUIRES coreneuron mpi python ${processor}
    PROCESSORS ${ringtest_mpi_ranks}
    COMMAND ${ringtest_special} -tstop 100 -coreneuron ${gpu_arg})
  nrn_add_test(
    GROUP external_ringtest
    NAME coreneuron_${processor}_mpi_python
    PRELOAD_SANITIZER
    REQUIRES coreneuron_shared mpi python ${processor}
    PROCESSORS ${ringtest_mpi_ranks}
    ENVIRONMENT NEURON_INIT_MPI=1
    COMMAND ${ringtest_python} -tstop 100 -coreneuron ${gpu_arg})
  nrn_add_test(
    GROUP external_ringtest
    NAME coreneuron_${processor}_mpi_offline
    REQUIRES coreneuron mpi python ${processor}
    CONFLICTS mpi_dynamic
    PROCESSORS ${ringtest_mpi_ranks}
    ENVIRONMENT OMP_NUM_THREADS=1 LIBSONATA_ZERO_BASED_GIDS=1
    PRECOMMAND ${ringtest_special} -tstop 0 -coreneuron -dumpmodel
    COMMAND ${ringtest_special_core} --mpi -d coredat/ -e 100 ${special_gpu_arg}
    OUTPUT asciispikes::out.dat)

  # Following suggestion on #1552, run a test with more threads than datasets.
  set(ringtest_num_threads 3)
  set(ringtest_num_datasets 2)
  math(EXPR ringtest_mpi_ranks_times_threads "${ringtest_mpi_ranks}*${ringtest_num_threads}")
  nrn_add_test(
    GROUP external_ringtest
    NAME coreneuron_${processor}_mpi_threads
    REQUIRES coreneuron mpi python ${processor}
    PROCESSORS ${ringtest_mpi_ranks_times_threads}
    ENVIRONMENT OMP_NUM_THREADS=${ringtest_num_threads}
    COMMAND ${ringtest_special} -tstop 100 -coreneuron -nt ${ringtest_num_datasets} ${gpu_arg})
  nrn_add_test(
    GROUP external_ringtest
    NAME coreneuron_${processor}_mpi_threads_python
    PRELOAD_SANITIZER
    REQUIRES coreneuron_shared mpi python ${processor}
    PROCESSORS ${ringtest_mpi_ranks_times_threads}
    ENVIRONMENT OMP_NUM_THREADS=${ringtest_num_threads} NEURON_INIT_MPI=1
    COMMAND ${ringtest_python} -tstop 100 -coreneuron -nt ${ringtest_num_datasets} ${gpu_arg})
endforeach()
# Step 3 -- add a job that compares the outputs of all the tests added in Step 2
nrn_add_test_group_comparison(
  GROUP external_ringtest
  REFERENCE_OUTPUT asciispikes::external/tests/ringtest/reference_data/spk1.100ms.std.ref)
