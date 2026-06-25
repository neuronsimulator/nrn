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
string(REPLACE ";" " " ringtest_special_str "${ringtest_special}")
string(REPLACE ";" " " ringtest_special_core_str "${ringtest_special_core}")

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

if(NRN_ENABLE_GPU)
  set(ringtest_gpu_native_patch
      ${PROJECT_SOURCE_DIR}/test/external/ringtest/ringtest-gpu-native.patch)
  set(ringtest_native_gpu_dir ${PROJECT_BINARY_DIR}/test/external_ringtest/neuron_gpu_native_mpi)
  if(EXISTS ${ringtest_gpu_native_patch} AND EXISTS ${ringtest_native_gpu_dir}/ringtest.py)
    execute_process(
      COMMAND patch -N -p1 -i ${ringtest_gpu_native_patch}
      WORKING_DIRECTORY ${ringtest_native_gpu_dir}
      RESULT_VARIABLE _ringtest_gpu_native_patch_result
      ERROR_VARIABLE _ringtest_gpu_native_patch_error)
    if(NOT _ringtest_gpu_native_patch_result EQUAL 0
       AND NOT _ringtest_gpu_native_patch_error MATCHES "Reversed.*patch.*already applied"
       AND NOT _ringtest_gpu_native_patch_error MATCHES "Skipping patch")
      message(WARNING "ringtest-gpu-native.patch: ${_ringtest_gpu_native_patch_error}")
    endif()
  endif()
  set(ringtest_native_gpu_mpi_script
      ${PROJECT_BINARY_DIR}/test/external_ringtest/neuron_gpu_native_mpi.sh)
  file(
    WRITE ${ringtest_native_gpu_mpi_script}
    "#!/bin/bash\n"
    "set -e\n"
    "export OMP_NUM_THREADS=1\n"
    "export NRN_GPU_BACKEND_TEST=native\n"
    "export NRN_GPU_PERMUTE=2\n"
    "export TMPDIR=\"${PROJECT_BINARY_DIR}/test/tmp/external_ringtest/neuron_gpu_native_mpi\"\n"
    "mkdir -p \"$TMPDIR\"\n"
    "OMP_NUM_THREADS=1 ${ringtest_special_str} -gpu-native -tstop 100\n")
  nrn_add_test(
    GROUP external_ringtest
    NAME neuron_gpu_native_mpi
    REQUIRES gpu mpi python mod_compatibility
    PROCESSORS ${ringtest_mpi_ranks}
    COMMAND bash ${ringtest_native_gpu_mpi_script}
    OUTPUT asciispikes::spk2.std)
  set(ringtest_native_gpu_mpi_gap_script
      ${PROJECT_BINARY_DIR}/test/external_ringtest/neuron_gpu_native_mpi_gap.sh)
  file(
    WRITE ${ringtest_native_gpu_mpi_gap_script}
    "#!/bin/bash\n"
    "set -e\n"
    "export OMP_NUM_THREADS=1\n"
    "export NRN_GPU_BACKEND_TEST=native\n"
    "export NRN_GPU_PERMUTE=2\n"
    "export TMPDIR=\"${PROJECT_BINARY_DIR}/test/tmp/external_ringtest/neuron_gpu_native_mpi_gap\"\n"
    "mkdir -p \"$TMPDIR\"\n"
    "OMP_NUM_THREADS=1 ${ringtest_special_str} -gpu-native -gap -tstop 100\n")
  nrn_add_test(
    GROUP external_ringtest
    NAME neuron_gpu_native_mpi_gap
    REQUIRES gpu mpi python mod_compatibility
    PROCESSORS ${ringtest_mpi_ranks}
    COMMAND bash ${ringtest_native_gpu_mpi_gap_script}
    OUTPUT asciispikes::spk2.std)
  set(ringtest_gap_ref_source
      ${PROJECT_SOURCE_DIR}/test/external/ringtest/reference_data/spk2.gap.100ms.std.ref)
  set(ringtest_gap_ref_build
      ${PROJECT_BINARY_DIR}/test/external/ringtest/reference_data/spk2.gap.100ms.std.ref)
  cpp_cc_build_time_copy(INPUT "${ringtest_gap_ref_source}" OUTPUT "${ringtest_gap_ref_build}")
  add_test(
    NAME external_ringtest::compare_neuron_gpu_native_mpi_gap
    COMMAND
      ${CMAKE_COMMAND} -E compare_files
      ${PROJECT_BINARY_DIR}/test/external_ringtest/neuron_gpu_native_mpi_gap/spk2.std
      ${ringtest_gap_ref_build}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  set_tests_properties(
    external_ringtest::compare_neuron_gpu_native_mpi_gap
    PROPERTIES DEPENDS external_ringtest::neuron_gpu_native_mpi_gap)
endif()

# creep into this file to add a nrn only test for ParallelContext.optimize_node_order(i)
nrn_add_test_group(
  NAME external_ringtest_nrn
  SUBMODULE tests/ringtest # git submodule where the relevant tests are defined
  ENVIRONMENT OMP_NUM_THREADS=1
  MODFILE_PATTERNS "mod/*.mod"
  SCRIPT_PATTERNS "*.py" "*.hoc")

nrn_add_test(
  GROUP external_ringtest_nrn
  NAME optim_node_order
  REQUIRES python
  COMMAND nrniv -python test_optimize_node_order.py -tstop 50 -rparm -npt 1)

foreach(processor cpu gpu)
  if("${processor}" STREQUAL "gpu")
    set(gpu_arg -gpu)
    set(special_gpu_arg --gpu)
    # Online embedded GPU via NEURON special + MPI can fail to load NVHPC temp fat objects
    # (pgcudafat*.o: cannot open shared object file). Run GPU via special-core instead, matching the
    # offline ringtest pattern that already uses special-core for --gpu.
    set(ringtest_gpu_mpi_script
        ${PROJECT_BINARY_DIR}/test/external_ringtest/coreneuron_gpu_mpi_special_core.sh)
    file(
      WRITE ${ringtest_gpu_mpi_script}
      "#!/bin/bash\n"
      "set -e\n"
      "OMP_NUM_THREADS=1 ${ringtest_special_str} -tstop 0 -coreneuron -dumpmodel\n"
      "OMP_NUM_THREADS=1 LIBSONATA_ZERO_BASED_GIDS=1 ${ringtest_special_core_str} --mpi -d coredat/ -e 100 --gpu\n"
      "cp out.dat spk2.std\n")
  else()
    set(gpu_arg)
    set(special_gpu_arg)
  endif()
  set(ringtest_corenrn_script
      ${PROJECT_BINARY_DIR}/test/external_ringtest/coreneuron_${processor}_mpi_offline_saverestore/coreneuron_${processor}_mpi_offline_saverestore.sh
  )
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
  if("${processor}" STREQUAL "gpu")
    nrn_add_test(
      GROUP external_ringtest
      NAME coreneuron_${processor}_mpi
      REQUIRES coreneuron mpi python ${processor}
      PROCESSORS ${ringtest_mpi_ranks}
      COMMAND bash ${ringtest_gpu_mpi_script})
  else()
    nrn_add_test(
      GROUP external_ringtest
      NAME coreneuron_${processor}_mpi
      REQUIRES coreneuron mpi python ${processor}
      PROCESSORS ${ringtest_mpi_ranks}
      COMMAND ${ringtest_special} -tstop 100 -coreneuron ${gpu_arg})
  endif()
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

nrn_add_test(
  GROUP external_ringtest
  NAME coreneuron_mpi_registermapping_directmode
  REQUIRES coreneuron mpi python
  PROCESSORS ${ringtest_mpi_ranks}
  ENVIRONMENT NEURON_INIT_MPI=1 LIBSONATA_ZERO_BASED_GIDS=1
  COMMAND ${ringtest_special} -tstop 100 -coreneuron -registermapping)
nrn_add_test(
  GROUP external_ringtest
  NAME coreneuron_mpi_registermapping_filemode
  REQUIRES coreneuron mpi python
  PROCESSORS ${ringtest_mpi_ranks}
  ENVIRONMENT NEURON_INIT_MPI=1 LIBSONATA_ZERO_BASED_GIDS=1
  COMMAND ${ringtest_special} -tstop 100 -coreneuron -registermapping -filemode)

# Step 3 -- add a job that compares the outputs of all the tests added in Step 2
nrn_add_test_group_comparison(
  GROUP external_ringtest
  REFERENCE_OUTPUT asciispikes::external/tests/ringtest/reference_data/spk1.100ms.std.ref)

if(CORENRN_ENABLE_REPORTING)
  find_program(H5DIFF_EXECUTABLE "h5diff")
  if(H5DIFF_EXECUTABLE)
    set(reference_path
        "${PROJECT_BINARY_DIR}/test/external/tests/ringtest/reference_data/soma.ref.h5")
    cpp_cc_build_time_copy(
      INPUT "${PROJECT_SOURCE_DIR}/external/tests/ringtest/reference_data/soma.ref.h5"
      OUTPUT "${reference_path}")
    foreach(testfolder coreneuron_mpi_registermapping_directmode
                       coreneuron_mpi_registermapping_filemode)
      add_test(NAME external_ringtest::compare_soma_report_${testfolder}
               COMMAND "h5diff" ${PROJECT_BINARY_DIR}/test/external_ringtest/${testfolder}/soma.h5
                       ${reference_path})
      set_tests_properties(external_ringtest::compare_soma_report_${testfolder}
                           PROPERTIES DEPENDS "external_ringtest::${testfolder}")
    endforeach()
  else()
    message(WARNING "Can not find h5diff, skip comparing the soma report for "
                    "external_ringtest::coreneuron_mpi_registermapping_directmode "
                    "external_ringtest::coreneuron_mpi_registermapping_filemode")
  endif()
endif()
