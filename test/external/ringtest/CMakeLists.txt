# Set up tests from the ringtest external repository
include(NeuronTestHelper)
# Step 1 -- define a group of configurations
nrn_add_test_group(
  NAME external_ringtest
  SUBMODULE tests/ringtest # git submodule where the relevant tests are defined
  MODFILE_PATTERNS "mod/*.mod"
  OUTPUT asciispikes::spk2.std # which output data to compare
  SCRIPT_PATTERNS "*.py" "*.hoc")

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
  PROCESSORS 2
  COMMAND
    ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special
    ${MPIEXEC_POSTFLAGS} -mpi -python ringtest.py -tstop 100)

nrn_add_test(
  GROUP external_ringtest
  NAME coreneuron_mpi
  REQUIRES coreneuron mpi python
  PROCESSORS 2
  COMMAND
    ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special
    ${MPIEXEC_POSTFLAGS} -mpi -python ringtest.py -tstop 100 -coreneuron)

nrn_add_test(
  GROUP external_ringtest
  NAME coreneuron_mpi_threads
  REQUIRES coreneuron mpi python
  PROCESSORS 2
  COMMAND
    ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS} special
    ${MPIEXEC_POSTFLAGS} -mpi -python ringtest.py -tstop 100 -coreneuron -nt 2)

# Step 3 -- add a job that compares the outputs of all the tests added in Step 2
nrn_add_test_group_comparison(
  GROUP external_ringtest
  REFERENCE_OUTPUT asciispikes::external/tests/ringtest/reference_data/spk1.100ms.std.ref)
