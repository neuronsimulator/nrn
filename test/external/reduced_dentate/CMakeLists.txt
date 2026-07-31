include(NeuronTestHelper)
nrn_add_test_group(
  CORENEURON
  NAME reduced_dentate
  SUBMODULE tests/reduced_dentate
  ENVIRONMENT HOC_LIBRARY_PATH=templates OMP_NUM_THREADS=1
  OUTPUT "asciispikes::results/dentatenet_spikeout_*.dat"
  MODFILE_PATTERNS "mechanisms/*.mod"
  SCRIPT_PATTERNS
    "commonutils.py"
    "dat/*.dat"
    "datasets/Control/*.dat"
    "datasets/Control/MPP/MPPspiketrain.dat"
    "*.hoc"
    "parameters/Control.hoc"
    "templates/*.hoc")

set(reduced_dentate_mpi_ranks 4)
set(reduced_dentate_sim_time 10)
# TODO/WARNING: the max_cells_per_type=100 option shrinks the model in a way that has no particular
# scientific meaning. This was done as a short-term measure to keep the CTest suite reasonably fast,
# but it should be revisited. Ideally we would use a small model based on a newer version of
# https://github.com/soltesz-lab/dentate.
set(reduced_dentate_neuron_prefix
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

# -----------------------------------------------------------------------------
# P3: NEURON-native GPU (ACC mechs; 4-rank via h.nrnmpi_init, not special -mpi).
# Local FetchContent Gfluct3.mod must be device-safe (no static _ran_compat in ACC).
# Spike parity vs checked-in sorted ref (same 400 as control max_cells=100, tstop=10).
# -----------------------------------------------------------------------------
if(NRN_ENABLE_GPU)
  set(reduced_dentate_run_native
      "${PROJECT_SOURCE_DIR}/test/external/reduced_dentate/run_dentate_native.py")
  nrn_add_test_group(
    NAME reduced_dentate_native
    SUBMODULE tests/reduced_dentate
    ENVIRONMENT HOC_LIBRARY_PATH=templates OMP_NUM_THREADS=1 NRN_GPU_BACKEND_TEST=native
                NRN_GPU_PERMUTE=2
    OUTPUT "asciispikes::results/dentatenet_spikeout_*.dat"
    MODFILE_PATTERNS "mechanisms/*.mod"
    NRNIVMODL_ARGS
      -nmodl
      "${CMAKE_BINARY_DIR}/bin/nmodl"
      -nmodlflags
      "passes --inline host --c acc --oacc"
    SCRIPT_PATTERNS
      "commonutils.py"
      "dat/*.dat"
      "datasets/Control/*.dat"
      "datasets/Control/MPP/MPPspiketrain.dat"
      "*.hoc"
      "parameters/Control.hoc"
      "templates/*.hoc")
  # Do not pass special -c with -python (unreliable); NRN_TEST_* env for launcher.
  nrn_add_test(
    GROUP reduced_dentate_native
    NAME neuron_gpu_native
    REQUIRES gpu mpi python
    PROCESSORS ${reduced_dentate_mpi_ranks}
    ENVIRONMENT HOC_LIBRARY_PATH=templates OMP_NUM_THREADS=1 NRN_GPU_BACKEND_TEST=native
                NRN_GPU_PERMUTE=2 NRN_TEST_TSTOP=${reduced_dentate_sim_time}
                NRN_TEST_MAX_CELLS=100
    COMMAND
      ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} ${reduced_dentate_mpi_ranks} ${MPIEXEC_OVERSUBSCRIBE}
      ${MPIEXEC_PREFLAGS} special ${MPIEXEC_POSTFLAGS} -notatty -python
      ${reduced_dentate_run_native})
  # REFERENCE_OUTPUT path is relative to repo root and is also copied under
  # build-gpu/test/<path>. Prefer external/tests/... (not test/...) so the
  # compare path is not doubled. Tracked source lives under test/external/;
  # stage into FetchContent tree + build tree for the compare job.
  set(_dentate_spk_ref_src
      "${PROJECT_SOURCE_DIR}/test/external/reduced_dentate/reference/dentatenet_spikeout.spk")
  set(_dentate_spk_ref_stage
      "${PROJECT_SOURCE_DIR}/external/tests/reduced_dentate/reference/dentatenet_spikeout.spk")
  file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/external/tests/reduced_dentate/reference")
  configure_file("${_dentate_spk_ref_src}" "${_dentate_spk_ref_stage}" COPYONLY)
  nrn_add_test_group_comparison(
    GROUP reduced_dentate_native
    REFERENCE_OUTPUT
      asciispikes::external/tests/reduced_dentate/reference/dentatenet_spikeout.spk)
endif()
