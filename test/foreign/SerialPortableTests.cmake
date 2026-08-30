# Serial portable integration tests for foreign (wheel) mode. Included from
# test/foreign/CMakeLists.txt after ForeignTestHelpers. MPI / CoreNEURON groups live in
# MpiCoreNeuronTests.cmake (M4).

set(_nrn_foreign_pytest_args --capture=tee-sys)

# ---------------------------------------------------------------------------
# pytest + datahandle + coverage (need pytest package)
# ---------------------------------------------------------------------------
if(_nrn_foreign_have_pytest)
  nrn_add_test_group(NAME pytest MODFILE_PATTERNS test/pytest/*.mod)
  nrn_foreign_note_nrnivmodl_group(pytest)
  nrn_add_test(
    GROUP pytest
    NAME basic_tests
    COMMAND "${NRN_FOREIGN_PYTHON}" -m pytest ${_nrn_foreign_pytest_args} "./test/pytest"
    SCRIPT_PATTERNS "test/pytest/*.json" "test/pytest/*.py")
  nrn_foreign_finalize_test(pytest basic_tests pytest)

  nrn_add_test_group(NAME datahandle MODFILE_PATTERNS test/datahandle/*.mod)
  nrn_foreign_note_nrnivmodl_group(datahandle)
  nrn_add_test(
    GROUP datahandle
    NAME datahandle_tests
    COMMAND "${NRN_FOREIGN_PYTHON}" -m pytest ${_nrn_foreign_pytest_args} ./test/datahandle
    SCRIPT_PATTERNS test/datahandle/*.py)
  nrn_foreign_finalize_test(datahandle datahandle_tests datahandle)

  nrn_add_test_group(NAME coverage_tests MODFILE_PATTERNS test/cover/mod/*.mod)
  nrn_foreign_note_nrnivmodl_group(coverage_tests)
  nrn_add_test(
    GROUP coverage_tests
    NAME cover_tests
    COMMAND "${NRN_FOREIGN_PYTHON}" -m pytest ${_nrn_foreign_pytest_args} ./test/cover
    SCRIPT_PATTERNS test/cover/*.py test/cover/*.json)
  nrn_foreign_finalize_test(coverage_tests cover_tests cover)

  # Pure Python unit tests under test/unit_tests/hoc_python (no modfiles).
  nrn_add_test_group(NAME unit_tests MODFILE_PATTERNS NONE)
  nrn_add_test(
    GROUP unit_tests
    NAME python_unit_tests
    COMMAND "${NRN_FOREIGN_PYTHON}" -m pytest ${_nrn_foreign_pytest_args}
            "${NRN_FOREIGN_SOURCE_ROOT}/test/unit_tests/hoc_python"
    SCRIPT_PATTERNS "test/unit_tests/hoc_python/*.py")
  nrn_foreign_finalize_test(unit_tests python_unit_tests unit)
endif()

# ---------------------------------------------------------------------------
# example_nmodl — share/examples/nrniv/nmodl via special
# ---------------------------------------------------------------------------
nrn_add_test_group(
  NAME example_nmodl
  MODFILE_PATTERNS *.mod *.inc
  SIM_DIRECTORY share/examples/nrniv/nmodl)
nrn_foreign_note_nrnivmodl_group(example_nmodl)

foreach(ext hoc py)
  if(ext STREQUAL "py" AND NOT _nrn_foreign_have_pytest)
    continue()
  endif()
  file(
    GLOB example_nmodl_scripts
    RELATIVE "${NRN_FOREIGN_SOURCE_ROOT}/share/examples/nrniv/nmodl"
    "${NRN_FOREIGN_SOURCE_ROOT}/share/examples/nrniv/nmodl/*.${ext}")
  foreach(example_script ${example_nmodl_scripts})
    get_filename_component(name "${example_script}" NAME_WLE)
    if(ext STREQUAL "py")
      # Match in-tree harness: python -m pytest <script.py>
      nrn_add_test(
        GROUP example_nmodl
        NAME ${name}_${ext}
        COMMAND "${NRN_FOREIGN_PYTHON}" -m pytest ${_nrn_foreign_pytest_args} "${example_script}"
        SCRIPT_PATTERNS "${example_script}" "${name}.ses")
    else()
      nrn_add_test(
        GROUP example_nmodl
        NAME ${name}_${ext}
        COMMAND special "${example_script}"
        SCRIPT_PATTERNS "${example_script}" "${name}.ses")
    endif()
    nrn_foreign_finalize_test(example_nmodl ${name}_${ext} example_nmodl)
  endforeach()
endforeach()

# ---------------------------------------------------------------------------
# hoctests — each script under test/hoctests/*
# ---------------------------------------------------------------------------
nrn_add_test_group(
  NAME hoctests
  MODFILE_PATTERNS *.mod *.inc
  SIM_DIRECTORY test/hoctests)
nrn_foreign_note_nrnivmodl_group(hoctests)
set(hoctest_utils expect_err.hoc)
foreach(ext hoc py)
  file(
    GLOB hoc_scripts
    RELATIVE "${NRN_FOREIGN_SOURCE_ROOT}/test/hoctests"
    "${NRN_FOREIGN_SOURCE_ROOT}/test/hoctests/*/*.${ext}")
  foreach(hoc_script ${hoc_scripts})
    get_filename_component(name "${hoc_script}" NAME_WLE)
    if(ext STREQUAL "py")
      # Match in-tree: plain python interpreter (not pytest, not special -python).
      nrn_add_test(
        GROUP hoctests
        NAME ${name}_${ext}
        COMMAND "${NRN_FOREIGN_PYTHON}" "${hoc_script}"
        SCRIPT_PATTERNS "${hoc_script}" "tests/${name}.json" ${hoctest_utils})
    else()
      nrn_add_test(
        GROUP hoctests
        NAME ${name}_${ext}
        COMMAND special "${hoc_script}"
        SCRIPT_PATTERNS "${hoc_script}" "tests/${name}.json" ${hoctest_utils})
    endif()
    nrn_foreign_finalize_test(hoctests ${name}_${ext} hoctests)
  endforeach()
endforeach()

# ---------------------------------------------------------------------------
# ringtest + connect_dend (HOC via foreign nrniv + RunHOCTest.cmake)
# ---------------------------------------------------------------------------
if(NOT NRN_FOREIGN_NRNIV STREQUAL "")
  nrn_foreign_cmake_env_path(_nrn_hoc_path "${_nrn_foreign_py_bindir}")
  set(_ring_dir "${CMAKE_BINARY_DIR}/test/ringtest")
  file(MAKE_DIRECTORY "${_ring_dir}")
  add_test(
    NAME foreign::ringtest
    COMMAND
      ${CMAKE_COMMAND} -E env "PYTHONPATH=" "${_nrn_hoc_path}" ${CMAKE_COMMAND}
      -Dhoc_library_path=${NRN_FOREIGN_SOURCE_ROOT}/test/ringtest -Dexecutable=${NRN_FOREIGN_NRNIV}
      -Dexec_arg=${NRN_FOREIGN_SOURCE_ROOT}/test/ringtest/ring.hoc -Dout_file=out.dat
      -Dref_file=${NRN_FOREIGN_SOURCE_ROOT}/test/ringtest/out.dat.ref -Dwork_dir=${_ring_dir} -P
      ${NRN_FOREIGN_SOURCE_ROOT}/cmake/RunHOCTest.cmake)
  set_tests_properties(foreign::ringtest PROPERTIES LABELS "foreign;serial;ringtest" TIMEOUT 120)

  set(_cd_dir "${CMAKE_BINARY_DIR}/test/hoc_tests/connect_dend")
  file(MAKE_DIRECTORY "${_cd_dir}")
  add_test(
    NAME foreign::connect_dend
    COMMAND
      ${CMAKE_COMMAND} -E env "PYTHONPATH=" "${_nrn_hoc_path}" ${CMAKE_COMMAND}
      -Dexecutable=${NRN_FOREIGN_NRNIV}
      -Dexec_arg=${NRN_FOREIGN_SOURCE_ROOT}/test/hoc_tests/connect_dend/connect_dend.hoc
      -Dout_file=cell3soma.dat
      -Dref_file=${NRN_FOREIGN_SOURCE_ROOT}/test/hoc_tests/connect_dend/cell3soma.dat.ref
      -Dwork_dir=${_cd_dir} -P ${NRN_FOREIGN_SOURCE_ROOT}/cmake/RunHOCTest.cmake)
  set_tests_properties(foreign::connect_dend PROPERTIES LABELS "foreign;serial;connect_dend"
                                                        TIMEOUT 120)
endif()

# ---------------------------------------------------------------------------
# RxD (serial) — needs RX3D in the wheel and test/rxd/testdata
# ---------------------------------------------------------------------------
if(NRN_ENABLE_RX3D AND _nrn_foreign_have_pytest)
  set(_rxd_testdata "${NRN_FOREIGN_SOURCE_ROOT}/test/rxd/testdata")
  if(NOT EXISTS "${_rxd_testdata}/.git" AND NOT EXISTS "${_rxd_testdata}/README")
    # Try to populate the submodule once at configure time.
    find_package(Git QUIET)
    if(GIT_FOUND AND EXISTS "${NRN_FOREIGN_SOURCE_ROOT}/.gitmodules")
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${NRN_FOREIGN_SOURCE_ROOT}" submodule update --init --
                test/rxd/testdata
        RESULT_VARIABLE _rxd_sub_rc
        OUTPUT_QUIET ERROR_QUIET)
    endif()
  endif()
  file(GLOB _rxd_data "${_rxd_testdata}/*")
  if(_rxd_data STREQUAL "")
    message(STATUS "Skipping rxdmod_tests: test/rxd/testdata not populated "
                   "(git submodule update --init -- test/rxd/testdata)")
  else()
    nrn_add_test_group(
      NAME rxdmod_tests
      MODFILE_PATTERNS test/rxd/*.mod
      SCRIPT_PATTERNS test/rxd/*.py test/rxd/*/*.py test/rxd/3d/*.asc test/rxd/testdata/**/*.dat)
    nrn_foreign_note_nrnivmodl_group(rxdmod_tests)
    nrn_add_test(
      GROUP rxdmod_tests
      NAME rxd_tests
      COMMAND "${NRN_FOREIGN_PYTHON}" -m pytest ${_nrn_foreign_pytest_args} ./test/rxd)
    nrn_foreign_finalize_test(rxdmod_tests rxd_tests rxd)
  endif()
endif()

# ---------------------------------------------------------------------------
# gjtests (serial part only; MPI variant deferred to M4)
# ---------------------------------------------------------------------------
if(_nrn_foreign_have_pytest)
  nrn_add_test_group(NAME gjtests MODFILE_PATTERNS test/gjtests/*.mod)
  nrn_foreign_note_nrnivmodl_group(gjtests)
  # pytest -k "not par" still imports test_par_gj.py during collection. That file runs main() and
  # h.quit() at module level (false ctest green). In-tree runs test_natrans.py as a script; gj_par
  # is mpiexec of test_par_gj.py.
  # Skip CN half of test_natrans.py. NEURON half still runs. CN psolve of this
  # script aborted on Linux/macOS wheels (#3866); dedicated CN rows passed.
  nrn_add_test(
    GROUP gjtests
    NAME gj_serial
    COMMAND "${NRN_FOREIGN_PYTHON}" test/gjtests/test_natrans.py
    SCRIPT_PATTERNS test/gjtests/test_natrans.py
    ENVIRONMENT NRN_FOREIGN_SKIP_CORENEURON=1)
  nrn_foreign_finalize_test(gjtests gj_serial gjtests)
endif()
