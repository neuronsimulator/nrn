set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

include_directories(${NRN_INCLUDE_DIRS})

# Add directory-level default compiler flags -- these should be added to all NEURON targets, but not
# targets from included projects like CoreNEURON and NMODL
add_compile_options(${NRN_COMPILE_FLAGS})
add_compile_definitions(${NRN_COMPILE_DEFS})
add_link_options(${NRN_LINK_FLAGS})
include(NeuronTestHelper)

# =============================================================================
# Test executables
# =============================================================================
add_executable(testneuron common/catch2_main.cpp unit_tests/basic.cpp
                          unit_tests/oc/hoc_interpreter.cpp)
set(catch2_targets testneuron)
if(NRN_ENABLE_THREADS)
  add_executable(nrn-benchmarks common/catch2_main.cpp benchmarks/threads/test_multicore.cpp)
  target_link_libraries(nrn-benchmarks Threads::Threads)
  list(APPEND catch2_targets nrn-benchmarks)
endif()
foreach(target ${catch2_targets})
  cpp_cc_configure_sanitizers(TARGET ${target})
  target_link_libraries(${target} Catch2::Catch2 nrniv_lib)
  if(NOT MINGW)
    target_link_libraries(${target} ${CMAKE_DL_LIBS})
  endif()
endforeach()

# =============================================================================
# Copy necessary hoc files to build directory if they have not been copied yet
# =============================================================================
add_dependencies(testneuron copy_share_lib_to_build)

# =============================================================================
# Unit tests  (testneuron, ...)
# =============================================================================
nrn_add_test_group(NAME unit_tests MODFILE_PATTERNS NONE)
nrn_add_test(
  GROUP unit_tests
  NAME testneuron
  COMMAND $<TARGET_FILE:testneuron>)
if(TARGET nrn-benchmarks AND NRN_ENABLE_PERFORMANCE_TESTS)
  nrn_add_test(
    GROUP unit_tests
    NAME benchmarks
    PROCESSORS 16
    COMMAND $<TARGET_FILE:nrn-benchmarks> --processors=16)
endif()
# =============================================================================
# Add ringtest
# =============================================================================
set(RINGTEST_DIR ${PROJECT_SOURCE_DIR}/test/ringtest)
add_test(
  ringtest
  ${CMAKE_COMMAND}
  -Dexecutable=${CMAKE_BINARY_DIR}/bin/nrniv
  -Dexec_arg=ring.hoc
  -Dout_file=out.dat
  -Dref_file=out.dat.ref
  -Dwork_dir=${RINGTEST_DIR}
  -P
  ${PROJECT_SOURCE_DIR}/cmake/RunHOCTest.cmake)
list(APPEND TESTS ringtest)

# =============================================================================
# Add small hoc test
# =============================================================================
set(HOCTEST_DIR ${PROJECT_SOURCE_DIR}/test/hoc_tests/connect_dend)
if(NRN_ENABLE_CORENEURON OR NRN_ENABLE_MOD_COMPATIBILITY)
  set(REF_FILE cell3soma.core.dat.ref)
else()
  set(REF_FILE cell3soma.dat.ref)
endif()
add_test(
  connect_dend
  ${CMAKE_COMMAND}
  -Dexecutable=${CMAKE_BINARY_DIR}/bin/nrniv
  -Dexec_arg=connect_dend.hoc
  -Dout_file=cell3soma.dat
  -Dref_file=${REF_FILE}
  -Dwork_dir=${HOCTEST_DIR}
  -P
  ${PROJECT_SOURCE_DIR}/cmake/RunHOCTest.cmake)
list(APPEND TESTS connect_dend)

nrn_add_test_group(NAME mpi_init MODFILE_PATTERNS NONE)
set(mpi_init_requires_mpiexec mpi)
set(mpi_init_prefix_mpiexec ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 1 ${MPIEXEC_PREFLAGS})
set(mpi_init_suffix_mpiexec ${MPIEXEC_POSTFLAGS})
foreach(mpiexec "" "_mpiexec")
  set(nrniv ${mpi_init_prefix${mpiexec}} nrniv ${mpi_init_suffix${mpiexec}})
  nrn_add_test(
    GROUP mpi_init
    NAME nrniv${mpiexec}_mpiopt
    REQUIRES ${mpi_init_requires${mpiexec}}
    COMMAND ${nrniv} -mpi -c "quit()")
  nrn_add_test(
    GROUP mpi_init
    NAME nrniv${mpiexec}_nrnmpi_init
    REQUIRES ${mpi_init_requires${mpiexec}}
    COMMAND ${nrniv} -c "nrnmpi_init()" -c "quit()")
  # See https://www.neuron.yale.edu/phpBB/viewtopic.php?t=4297 and links therein.
  if(NRN_HAVE_OPENMPI2_OR_LESS AND NOT NRN_ENABLE_MPI_DYNAMIC)
    set(python ${mpi_init_prefix${mpiexec}} nrniv ${mpi_init_suffix${mpiexec}} -python)
  else()
    set(python ${mpi_init_prefix${mpiexec}} ${preload_sanitizer${mpiexec}}
               ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${mpi_init_suffix${mpiexec}})
  endif()
  # Note that we don't collect coverage from these tests...
  nrn_add_test(
    GROUP mpi_init
    NAME python${mpiexec}_nrnmpi_init
    PRELOAD_SANITIZER
    REQUIRES python ${mpi_init_requires${mpiexec}}
    COMMAND ${python} -c "from neuron import h$<SEMICOLON> h.nrnmpi_init()$<SEMICOLON> h.quit()")
  nrn_add_test(
    GROUP mpi_init
    NAME python${mpiexec}_mpienv
    PRELOAD_SANITIZER
    REQUIRES python ${mpi_init_requires${mpiexec}}
    ENVIRONMENT NEURON_INIT_MPI=1
    COMMAND ${python} -c "from neuron import h$<SEMICOLON> h.quit()")
endforeach()

# ============================================================================================
# Add python tests. pytest is a compulsory dependency when Python and tests are enabled, while
# coverage is an optional dependency.
# ============================================================================================
if(NRN_ENABLE_PYTHON)
  if(NRN_ENABLE_COVERAGE AND COVERAGE_FOUND)
    # Tests that expect to be driven by pytest should use nrn_add_pytest(...). Tests that expect to
    # be run directly as Python scripts should use ${PYTHON_EXECUTABLE} ${coverage} path/to/file.py
    # to enable code coverage. The -p option to coverage run appends the host and pid to the
    # .coverage filename, fixing MPI execution.
    set(coverage -m coverage run -p)
  endif()
  # This includes most pytest-driven NEURON tests. MOD files are compiled for CoreNEURON if
  # CoreNEURON is enabled at compile time. Some tests dynamically enable CoreNEURON when it is
  # available.
  nrn_add_test_group(
    CORENEURON
    NAME pynrn
    MODFILE_PATTERNS *.mod
    SIM_DIRECTORY test/pynrn)
  # In case of multiple Python versions, run these tests with all of them
  foreach(val RANGE ${NRN_PYTHON_ITERATION_LIMIT})
    list(GET NRN_PYTHON_EXECUTABLES ${val} exe)
    list(GET NRN_PYTHON_VERSIONS ${val} pyver)
    # FIXME how do we say which Python to use???
    nrn_add_pytest(
      GROUP pynrn
      NAME basic_tests_py${pyver}
      SCRIPT_PATTERNS *.json *.py)
  endforeach()

  # This test is steered via the NRN_CORENEURON_ENABLE. TODO make it just dynamically test
  # CoreNEURON if it's available.
  nrn_add_pytest(
    GROUP pynrn
    NAME explicit_coreneuron
    REQUIRES coreneuron
    SCRIPT_PATTERNS test_version_macros.py
    ENVIRONMENT NRN_CORENEURON_ENABLE=true)
  # This test only runs on GPU if explicitly configured with CORENRN_ENABLE_GPU. TODO make it less
  # hacky.
  nrn_add_pytest(
    GROUP pynrn
    NAME explicit_coreneuron_gpu
    REQUIRES coreneuron gpu
    SCRIPT_PATTERNS test_fast_imem.py
    ENVIRONMENT CORENRN_ENABLE_GPU=true)
  # Also execute some test/pynrn/ tests under MPI for additional coverage. If the tests were all
  # well-behaved and left the global state clean hen there would be no reason to have multiple
  # nrn_add_pytest calls here. Only do this with the default Python.
  nrn_add_pytest(
    GROUP pynrn
    NAME basic_tests_parallel_netpar
    MPI_RANKS 2
    SCRIPT_PATTERNS ring.py test_netpar.py)
  nrn_add_pytest(
    GROUP pynrn
    NAME basic_tests_parallel_nrntest
    MPI_RANKS 2
    SCRIPT_PATTERNS test_nrntest_fast.json test_nrntest_fast.py)
  nrn_add_pytest(
    GROUP pynrn
    NAME basic_tests_parallel_partrans
    MPI_RANKS 2
    SCRIPT_PATTERNS test_partrans.py)

  # Add some tests that are specifically aimed at NEURON - Python integration, covering different
  # ways NEURON can be made to execute Python code.
  add_subdirectory(pyinit)

  # Mostly to increase coverage, more pytest-driven NEURON tests. Not compiled for CoreNEURON.
  nrn_add_test_group(
    NAME coverage_tests
    MODFILE_PATTERNS mod/*.mod
    SIM_DIRECTORY test/cover)
  nrn_add_pytest(
    GROUP coverage_tests
    NAME cover_tests
    SCRIPT_PATTERNS *.py *.json)

  # Example HOC and Python scripts, *not* driven by pytest. Not compiled for CoreNEURON. We assume
  # that ${PYTHON_EXECUTABLE} can be used, which might be wrong if NEURON is linked statically?
  nrn_add_test_group(
    NAME example_nmodl
    MODFILE_PATTERNS *.mod *.inc
    SIM_DIRECTORY share/examples/nrniv/nmodl)
  set(py_exe ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${coverage})
  set(py_preload PRELOAD_SANITIZER)
  set(hoc_exe special)
  foreach(ext hoc py)
    file(
      GLOB example_nmodl_scripts
      RELATIVE "${PROJECT_SOURCE_DIR}/share/examples/nrniv/nmodl"
      "${PROJECT_SOURCE_DIR}/share/examples/nrniv/nmodl/*.${ext}")
    foreach(example_script ${example_nmodl_scripts})
      get_filename_component(name "${example_script}" NAME_WLE)
      nrn_add_test(
        GROUP example_nmodl
        NAME ${name}_${ext} ${${ext}_preload} # PRELOAD_SANITIZER for Python
        COMMAND ${${ext}_exe} "${example_script}"
        SCRIPT_PATTERNS "${example_script}" "${name}.ses")
    endforeach()
  endforeach()

  # Independent run of each hoc and python file in test/hoc/* folders. Same logic/limitations as
  # example_nmodl.
  nrn_add_test_group(
    NAME hoctests
    MODFILE_PATTERNS *.mod
    SIM_DIRECTORY test/hoctests)
  set(hoctest_utils expect_err.hoc)
  foreach(ext hoc py)
    file(
      GLOB hoc_scripts
      RELATIVE "${PROJECT_SOURCE_DIR}/test/hoctests"
      "${PROJECT_SOURCE_DIR}/test/hoctests/*/*.${ext}")
    foreach(hoc_script ${hoc_scripts})
      get_filename_component(name "${hoc_script}" NAME_WLE)
      nrn_add_test(
        GROUP hoctests
        NAME ${name}_${ext} ${${ext}_preload} # PRELOAD_SANITIZER for Python
        COMMAND ${${ext}_exe} "${hoc_script}"
        SCRIPT_PATTERNS "${hoc_script}" "tests/${name}.json" ${${ext}test_utils})
    endforeach()
  endforeach()

  if(NRN_ENABLE_RX3D)
    # SIM_DIRECTORY test/rxd would be less repetitive, but rxd is overly fussy about what directory
    # it runs in and where the x86_64 output from nrnivmodl sits in the tree
    nrn_add_test_group(
      NAME rxdmod_tests
      MODFILE_PATTERNS test/rxd/*.mod
      SCRIPT_PATTERNS test/rxd/*.py test/rxd/*/*.py test/rxd/3d/*.asc test/rxd/testdata/**/*.dat)
    # These tests include comparisons against saved data, which appears to have been generated using
    # GCC. Other compilers produce larger differences.
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
      set(change_test_tolerance NRN_RXD_TEST_TOLERANCE=1e-8)
    elseif(NRN_HAVE_NVHPC_COMPILER)
      set(change_test_tolerance NRN_RXD_TEST_TOLERANCE=1e-4)
    endif()
    # Run all the RxD tests in a single pytest run
    nrn_add_pytest(
      GROUP rxdmod_tests
      NAME rxd_tests
      ENVIRONMENT ${change_test_tolerance})
    if(NRN_ENABLE_MPI)
      nrn_find_python_module(MODULE mpi4py)
      if(MPI4PY_FOUND)
        # Run all the RxD tests with mpiexec -n 1 too
        nrn_add_pytest(
          GROUP rxdmod_tests
          NAME rxd_mpi_tests
          ENVIRONMENT ${change_test_tolerance}
          MPI_RANKS 1
          PYTEST_ARGS ./test/rxd/ --mpi)
      endif()
    endif()
  endif()
  # We are not currently collecting Python coverage for tests in this group, which are not all
  # pytest-compatible. TODO: clean these up for inclusion in basic_tests_parallel*
  nrn_add_test_group(
    CORENEURON
    NAME parallel
    MODFILE_PATTERNS test/pynrn/*.mod)
  nrn_add_test(
    GROUP parallel
    NAME subworld
    PROCESSORS 6
    REQUIRES mpi
    SCRIPT_PATTERNS test/parallel_tests/test_subworld.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            nrniv ${MPIEXEC_POSTFLAGS} -mpi -python test/parallel_tests/test_subworld.py)
  nrn_add_test(
    GROUP parallel
    NAME bas
    PROCESSORS 2
    REQUIRES mpi
    SCRIPT_PATTERNS test/parallel_tests/test_bas.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            nrniv ${MPIEXEC_POSTFLAGS} -mpi -python test/parallel_tests/test_bas.py)
  # CoreNEURON's reports require MPI and segfault if it is not initialised. This is a crude
  # workaround.
  if(CORENRN_ENABLE_REPORTING)
    set(nrnpython_mpi_env NEURON_INIT_MPI=1)
    # see #1619, required when coreneuron is linked to sonata library
    set(sonata_zero_gid_env LIBSONATA_ZERO_BASED_GIDS=1)
  endif()
  # External coreneuron can be used for testing but for simplicity we are testing only submodule
  # builds (in near future we want to support only internal builds anyway). This test uses the
  # standard NEURON installation that does not have a `special` statically linked against
  # CoreNEURON, so we cannot run this in static builds.
  if(NOT nrn_using_ext_corenrn AND CORENRN_ENABLE_SHARED)
    nrn_add_test_group(
      CORENEURON
      NAME coreneuron_standalone
      MODFILE_PATTERNS NONE # NONE => nrnivmodl is not run
    )
    nrn_add_pytest(
      GROUP coreneuron_standalone
      NAME nrnivmodl_not_run
      REQUIRES coreneuron
      SCRIPT_PATTERNS test/coreneuron/test_psolve.py)
  endif()

  # Build and test examples from the NMODL language documentation. First, build and test all of them
  # for NEURON, using pytest.
  nrn_add_test_group(
    NAME nmodl_tests
    SCRIPT_PATTERNS test/nmodl/*.py
    MODFILE_PATTERNS "docs/nmodl/mod/*.mod")
  nrn_add_pytest(GROUP nmodl_tests NAME neuron)

  # Second, build and test the subset that are compatible with CoreNEURON.
  nrn_add_test_group(
    CORENEURON # build MOD files for CoreNEURON
    NAME nmodl_tests_coreneuron
    # Not: test_disc.py test_function_table.py test_kinetic.py
    SCRIPT_PATTERNS test/nmodl/test_table.py
    # Not: cadif.mod, disc.mod, k3st.mod, variabletypes.mod
    MODFILE_PATTERNS docs/nmodl/mod/fornetcon.mod docs/nmodl/mod/hhwatch.mod
                     docs/nmodl/mod/table.mod)
  # These require CoreNEURON and GPU support to be steered via environment variables
  nrn_add_pytest(
    GROUP nmodl_tests_coreneuron
    NAME cpu
    REQUIRES coreneuron
    ENVIRONMENT ${sonata_zero_gid_env} ${nrnpython_mpi_env} NRN_CORENEURON_ENABLE=true)
  nrn_add_pytest(
    GROUP nmodl_tests_coreneuron
    NAME gpu
    REQUIRES coreneuron gpu
    ENVIRONMENT ${sonata_zero_gid_env} ${nrnpython_mpi_env} NRN_CORENEURON_ENABLE=true
                CORENRN_ENABLE_GPU=true)

  # TODO test/gjtests is not executed?

  # Build CoreNEURON tests and execute them on both CPU and GPU, steering that by hoping everything
  # respects the CORENRN_ENABLE_GPU environment variable. See
  # https://github.com/neuronsimulator/nrn/issues/1792 ...
  nrn_add_test_group(
    CORENEURON
    NAME coreneuron_modtests
    MODFILE_PATTERNS "mod files/*.mod" "mod files/axial.inc" ../pynrn/unitstest.mod
                     ../gjtests/natrans.mod
    SIM_DIRECTORY test/coreneuron)
  # This test needs more ranks to make sense, but there's no need to run it on GPU
  nrn_add_pytest(
    GROUP coreneuron_modtests
    NAME mpi_6ranks
    REQUIRES coreneuron
    MPI_RANKS 6
    SCRIPT_PATTERNS test_subworlds.py)
  set(coreneuron_modtests_gpu_env CORENRN_ENABLE_GPU=true)
  foreach(processor cpu gpu)
    set(processor_env ${coreneuron_modtests_${processor}_env})
    set(modtests_processor_env ${processor_env} ${sonata_zero_gid_env})
    # Everything except test_subworlds.py on CPU|GPU, without MPI
    nrn_add_pytest(
      GROUP coreneuron_modtests
      NAME ${processor}
      REQUIRES coreneuron ${processor}
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env}
      SCRIPT_PATTERNS
        test_ba.py
        test_datareturn.py
        test_direct.py
        test_fornetcon.py
        test_inputpresyn.py
        test_netmove.py
        test_pointer.py
        test_spikes.py
        test_psolve.py
        test_units.py
        test_watchrange.py)
    # One HOC test. It can even run on GPU.
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME direct_hoc_${processor}
      REQUIRES coreneuron ${processor}
      SCRIPT_PATTERNS test_direct.hoc
      ENVIRONMENT ${processor_env}
      COMMAND special -notatty -mpi test_direct.hoc)
    # Run this test in file mode too. TODO: this should be handled internally...
    nrn_add_pytest(
      GROUP coreneuron_modtests
      NAME spikes_file_mode_py_${processor}
      REQUIRES coreneuron ${processor}
      ENVIRONMENT ${modtests_processor_env} ${nrnpython_mpi_env} NRN_TEST_SPIKES_FILE_MODE=1
      SCRIPT_PATTERNS test_spikes.py)
    if(NRN_ENABLE_MPI)
      # Run this test using mpi4py too. TODO: is this really needed given that we have the mpi_init
      # tests?
      nrn_find_python_module(MODULE mpi4py)
      if(mpi4py_FOUND)
        nrn_add_pytest(
          GROUP coreneuron_modtests
          NAME spikes_mpi_py_${processor}
          REQUIRES coreneuron ${processor}
          MPI_RANKS 2
          ENVIRONMENT ${processor_env} NRN_TEST_SPIKES_MPI4PY=1
          SCRIPT_PATTERNS test_spikes.py)
      endif()
      # Run some of the tests with 2 MPI ranks too
      nrn_add_pytest(
        GROUP coreneuron_modtests
        NAME mpi_2ranks_${processor}
        REQUIRES coreneuron ${processor}
        MPI_RANKS 2
        ENVIRONMENT ${processor_env} NRN_TEST_SPIKES_NRNMPI_INIT=1 NRN_TEST_SPIKES_FILE_MODE=1
        SCRIPT_PATTERNS test_spikes.py test_inputpresyn.py)
    endif()
  endforeach()

  if(NRN_ENABLE_MUSIC)
    set(music_launch_command
        "${MPIEXEC_NAME}  ${MPIEXEC_NUMPROC_FLAG}  2 ${MPIEXEC_OVERSUBSCRIBE}   ${MPIEXEC_PREFLAGS} music"
    )
    nrn_add_test_group(
      NAME nrnmusic
      ENVIRONMENT MUSIC_LIBDIR=${MUSIC_LIBDIR}
      ENVIRONMENT MPIEXEC_COMMAND=${music_launch_command}
      MODFILE_PATTERNS NONE)
    nrn_add_test(
      GROUP nrnmusic
      NAME music_tests
      PROCESSORS 2
      COMMAND ${NRN_DEFAULT_PYTHON_EXECUTABLE} ${coverage} test/music_tests/runtests.py
      SCRIPT_PATTERNS test/music_tests/*.music test/music_tests/*.py)
  endif()
endif()

# ============================================
# Test modlunit
# ============================================
function(add_modlunit_test mod_file)
  get_filename_component(mod_file_basename "${mod_file}" NAME_WE)
  add_test(
    NAME modlunit_${mod_file_basename}
    COMMAND modlunit "${mod_file}"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  list(APPEND TESTS modlunit_${mod_file_basename})
  set(TESTS
      "${TESTS}"
      PARENT_SCOPE)
endfunction()
add_modlunit_test("${PROJECT_SOURCE_DIR}/test/pynrn/unitstest.mod")
add_modlunit_test("${PROJECT_SOURCE_DIR}/src/nrnoc/hh.mod")
add_modlunit_test("${PROJECT_SOURCE_DIR}/src/nrnoc/stim.mod")
add_modlunit_test("${PROJECT_SOURCE_DIR}/src/nrnoc/pattern.mod")
set_property(TEST modlunit_pattern PROPERTY WILL_FAIL ON)

set_tests_properties(${TESTS} PROPERTIES ENVIRONMENT "${NRN_RUN_FROM_BUILD_DIR_ENV}")
cpp_cc_configure_sanitizers(TEST ${TESTS})
# If profiling is enabled, run ringtest with profiler
if(NRN_ENABLE_PROFILING)
  set(TEST_PROFILING_ENV ${NRN_RUN_FROM_BUILD_DIR_ENV})
  list(APPEND TEST_PROFILING_ENV "CALI_CONFIG=runtime-report,calc.inclusive")
  set_tests_properties(ringtest PROPERTIES ENVIRONMENT "${TEST_PROFILING_ENV}")
endif()

# Add tests that are configured using external repositories
add_subdirectory(external)
