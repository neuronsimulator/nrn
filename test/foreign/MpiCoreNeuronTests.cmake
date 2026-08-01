# MPI and CoreNEURON integration tests for foreign (wheel) mode (M4). Included after
# SerialPortableTests.cmake. Tests are registered only when the wheel reports the feature and the
# host has the needed tools (mpiexec, etc.).

set(_nrn_foreign_pytest_args --capture=tee-sys)

# ---------------------------------------------------------------------------
# MPI smoke (no mechanism compile)
# ---------------------------------------------------------------------------
if(NRN_ENABLE_MPI AND NOT MPIEXEC_NAME STREQUAL "")
  nrn_add_test_group(NAME mpi_init MODFILE_PATTERNS NONE)

  nrn_add_test(
    GROUP mpi_init
    NAME nrniv_mpiopt
    REQUIRES mpi
    COMMAND nrniv -mpi -c "quit()")
  nrn_foreign_finalize_test(mpi_init nrniv_mpiopt mpi)

  nrn_add_test(
    GROUP mpi_init
    NAME nrniv_nrnmpi_init
    REQUIRES mpi
    COMMAND nrniv -c "nrnmpi_init()" -c "quit()")
  nrn_foreign_finalize_test(mpi_init nrniv_nrnmpi_init mpi)

  nrn_add_test(
    GROUP mpi_init
    NAME python_nrnmpi_init
    REQUIRES python mpi
    COMMAND "${NRN_FOREIGN_PYTHON}" -c
            "from neuron import h$<SEMICOLON> h.nrnmpi_init()$<SEMICOLON> h.quit()")
  nrn_foreign_finalize_test(mpi_init python_nrnmpi_init mpi)

  nrn_add_test(
    GROUP mpi_init
    NAME mpiexec_nrniv
    REQUIRES mpi
    PROCESSORS 2
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            nrniv ${MPIEXEC_POSTFLAGS} -mpi -c "quit()")
  nrn_foreign_finalize_test(mpi_init mpiexec_nrniv mpi)

  nrn_add_test(
    GROUP mpi_init
    NAME mpiexec_python
    REQUIRES python mpi
    PROCESSORS 2
    COMMAND
      ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
      "${NRN_FOREIGN_PYTHON}" ${MPIEXEC_POSTFLAGS} -c
      "from neuron import h$<SEMICOLON> h.nrnmpi_init()$<SEMICOLON> h.quit()")
  nrn_foreign_finalize_test(mpi_init mpiexec_python mpi)
endif()

# ---------------------------------------------------------------------------
# Parallel integration (NEURON MPI, shared mod build with pytest_coreneuron)
# ---------------------------------------------------------------------------
if(NRN_ENABLE_MPI AND NOT MPIEXEC_NAME STREQUAL "")
  nrn_add_test_group(NAME parallel MODFILE_PATTERNS test/pytest_coreneuron/*.mod)
  nrn_foreign_note_nrnivmodl_group(parallel)

  nrn_add_test(
    GROUP parallel
    NAME bas
    PROCESSORS 2
    REQUIRES mpi
    SCRIPT_PATTERNS test/parallel_tests/test_bas.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            nrniv ${MPIEXEC_POSTFLAGS} -mpi -python test/parallel_tests/test_bas.py)
  nrn_foreign_finalize_test(parallel bas mpi parallel)

  nrn_add_test(
    GROUP parallel
    NAME partrans
    PROCESSORS 2
    REQUIRES mpi
    SCRIPT_PATTERNS test/pytest_coreneuron/test_partrans.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            nrniv ${MPIEXEC_POSTFLAGS} -mpi -python test/pytest_coreneuron/test_partrans.py)
  nrn_foreign_finalize_test(parallel partrans mpi parallel)

  nrn_add_test(
    GROUP parallel
    NAME netpar
    PROCESSORS 2
    REQUIRES mpi
    SCRIPT_PATTERNS test/pytest_coreneuron/test_hoc_po.py test/pytest_coreneuron/test_netpar.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            nrniv ${MPIEXEC_POSTFLAGS} -mpi -python test/pytest_coreneuron/test_netpar.py)
  nrn_foreign_finalize_test(parallel netpar mpi parallel)

  nrn_add_test(
    GROUP parallel
    NAME subworld
    PROCESSORS 6
    REQUIRES mpi
    SCRIPT_PATTERNS test/parallel_tests/test_subworld.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 6 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            nrniv ${MPIEXEC_POSTFLAGS} -mpi -python test/parallel_tests/test_subworld.py)
  nrn_foreign_finalize_test(parallel subworld mpi parallel)

  if(_nrn_foreign_have_pytest)
    string(JOIN " " _pytest_arg_string ${_nrn_foreign_pytest_args})
    nrn_add_test(
      GROUP parallel
      NAME nrntest_fast
      PROCESSORS 2
      REQUIRES mpi
      ENVIRONMENT "NRN_PYTEST_ARGS=${_pytest_arg_string}"
      SCRIPT_PATTERNS
        test/pytest_coreneuron/run_pytest.py test/pytest_coreneuron/test_nrntest_fast.json
        test/pytest_coreneuron/test_nrntest_fast.py
      COMMAND
        ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
        special ${MPIEXEC_POSTFLAGS} -mpi -python test/pytest_coreneuron/run_pytest.py)
    nrn_foreign_finalize_test(parallel nrntest_fast mpi parallel pytest)
  endif()
endif()

# ---------------------------------------------------------------------------
# pytest_coreneuron — serial pytest + CoreNEURON-enabled nrnivmodl
# ---------------------------------------------------------------------------
if(NRN_ENABLE_CORENEURON AND _nrn_foreign_have_pytest)
  nrn_add_test_group(
    CORENEURON
    NAME pytest_coreneuron
    MODFILE_PATTERNS test/pytest_coreneuron/*.mod)
  nrn_foreign_note_nrnivmodl_group(pytest_coreneuron)
  nrn_add_test(
    GROUP pytest_coreneuron
    NAME basic_tests
    REQUIRES coreneuron
    COMMAND "${NRN_FOREIGN_PYTHON}" -m pytest ${_nrn_foreign_pytest_args} "./test/pytest_coreneuron"
    SCRIPT_PATTERNS "test/pytest_coreneuron/*.json" "test/pytest_coreneuron/*.py")
  nrn_foreign_finalize_test(pytest_coreneuron basic_tests coreneuron pytest)
endif()

# ---------------------------------------------------------------------------
# CoreNEURON mechanism tests (CPU path; shared-library wheel layout)
# ---------------------------------------------------------------------------
if(NRN_ENABLE_CORENEURON)
  # Launchers match the shared CoreNEURON wheel case (python/nrniv load special mechs).
  if(CORENRN_ENABLE_SHARED)
    set(_cn_launch_py "${NRN_FOREIGN_PYTHON}" -m pytest ${_nrn_foreign_pytest_args})
    set(_cn_launch_hoc nrniv)
    set(_cn_launch_py_mpi
        ${MPIEXEC_NAME}
        ${MPIEXEC_NUMPROC_FLAG}
        2
        ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS}
        "${NRN_FOREIGN_PYTHON}"
        ${MPIEXEC_POSTFLAGS})
  else()
    set(_cn_launch_py special -notatty -mpi -python)
    set(_cn_launch_hoc special -notatty -mpi)
    set(_cn_launch_py_mpi
        ${MPIEXEC_NAME}
        ${MPIEXEC_NUMPROC_FLAG}
        2
        ${MPIEXEC_OVERSUBSCRIBE}
        ${MPIEXEC_PREFLAGS}
        special
        ${MPIEXEC_POSTFLAGS}
        -notatty
        -python)
  endif()

  if(CORENRN_ENABLE_SHARED AND _nrn_foreign_have_pytest)
    nrn_add_test_group(
      CORENEURON
      NAME coreneuron_standalone
      MODFILE_PATTERNS NONE)
    nrn_add_test(
      GROUP coreneuron_standalone
      NAME test_psolve
      REQUIRES coreneuron
      SCRIPT_PATTERNS test/coreneuron/test_psolve.py
      COMMAND ${_cn_launch_py} test/coreneuron/test_psolve.py)
    nrn_foreign_finalize_test(coreneuron_standalone test_psolve coreneuron)
  endif()

  nrn_add_test_group(
    CORENEURON
    NAME coreneuron_modtests
    SCRIPT_PATTERNS test/coreneuron/test_spikes.py
    MODFILE_PATTERNS
      "test/coreneuron/mod files/*.mod" "test/coreneuron/mod files/axial.inc"
      test/pytest_coreneuron/unitstest.mod test/pytest_coreneuron/version_macros.mod
      test/gjtests/natrans.mod)
  nrn_foreign_note_nrnivmodl_group(coreneuron_modtests)

  if(_nrn_foreign_have_pytest)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME version_macros
      REQUIRES coreneuron
      SCRIPT_PATTERNS test/pytest_coreneuron/test_version_macros.py
      ENVIRONMENT NRN_CORENEURON_ENABLE=true
      COMMAND ${_cn_launch_py} test/pytest_coreneuron/test_version_macros.py)
    nrn_foreign_finalize_test(coreneuron_modtests version_macros coreneuron)

    # CPU-only CoreNEURON suite (GPU left for a later milestone).
    foreach(
      _cn_case
      fornetcon:test/coreneuron/test_fornetcon.py
      direct:test/coreneuron/test_direct.py
      spikes:test/coreneuron/test_spikes.py
      fast_imem:test/pytest_coreneuron/test_fast_imem.py
      datareturn:test/coreneuron/test_datareturn.py
      units:test/coreneuron/test_units.py
      netmove:test/coreneuron/test_netmove.py
      pointer:test/coreneuron/test_pointer.py
      watchrange:test/coreneuron/test_watchrange.py
      psolve:test/coreneuron/test_psolve.py)
      string(REPLACE ":" ";" _cn_pair "${_cn_case}")
      list(GET _cn_pair 0 _cn_name)
      list(GET _cn_pair 1 _cn_script)
      nrn_add_test(
        GROUP coreneuron_modtests
        NAME ${_cn_name}_py_cpu
        REQUIRES coreneuron cpu
        SCRIPT_PATTERNS ${_cn_script}
        COMMAND ${_cn_launch_py} ${_cn_script})
      nrn_foreign_finalize_test(coreneuron_modtests ${_cn_name}_py_cpu coreneuron)
    endforeach()

    nrn_add_test(
      GROUP coreneuron_modtests
      NAME direct_hoc_cpu
      REQUIRES coreneuron cpu
      SCRIPT_PATTERNS test/coreneuron/test_direct.hoc
      COMMAND ${_cn_launch_hoc} test/coreneuron/test_direct.hoc)
    nrn_foreign_finalize_test(coreneuron_modtests direct_hoc_cpu coreneuron)
  endif()

  # MPI + CoreNEURON
  if(NRN_ENABLE_MPI
     AND NOT MPIEXEC_NAME STREQUAL ""
     AND _nrn_foreign_have_pytest)
    nrn_add_test(
      GROUP coreneuron_modtests
      NAME inputpresyn_py
      REQUIRES coreneuron mpi
      SCRIPT_PATTERNS test/coreneuron/test_inputpresyn.py
      PROCESSORS 2
      COMMAND ${_cn_launch_py_mpi} test/coreneuron/test_inputpresyn.py)
    nrn_foreign_finalize_test(coreneuron_modtests inputpresyn_py mpi coreneuron)
  endif()
endif()

# ---------------------------------------------------------------------------
# Gap junction MPI test (serial gj covered in M3; same mod group)
# ---------------------------------------------------------------------------
if(NRN_ENABLE_MPI
   AND NOT MPIEXEC_NAME STREQUAL ""
   AND _nrn_foreign_have_pytest)
  # Group + nrnivmodl already registered in SerialPortableTests.cmake when pytest exists.
  nrn_add_test(
    GROUP gjtests
    NAME gj_par
    REQUIRES mpi
    PROCESSORS 2
    SCRIPT_PATTERNS test/gjtests/test_par_gj.py
    COMMAND ${MPIEXEC_NAME} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_OVERSUBSCRIBE} ${MPIEXEC_PREFLAGS}
            "${NRN_FOREIGN_PYTHON}" ${MPIEXEC_POSTFLAGS} test/gjtests/test_par_gj.py)
  nrn_foreign_finalize_test(gjtests gj_par mpi gjtests)
endif()
