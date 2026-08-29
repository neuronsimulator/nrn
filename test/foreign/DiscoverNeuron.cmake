# Discover a foreign (installed) NEURON via NRN_FOREIGN_PYTHON and probe_neuron.py.
#
# PATH and PYTHONPATH use ';' on Windows and ':' on Unix. Joining with ':' made shutil.which miss
# nrnivmodl.cmd in the venv Scripts directory.
#
# Sets (CACHE INTERNAL unless noted): NRN_FOREIGN_PYTHON              - interpreter used for
# discovery (user-facing CACHE) NRN_FOREIGN_NEURON_VERSION NRN_FOREIGN_NEURON_VERSION_FULL
# NRN_FOREIGN_NEURON_GIT_SHA NRN_FOREIGN_NEURON_FILE NRN_FOREIGN_NRNIV NRN_FOREIGN_NRNIVMODL
# NRN_FOREIGN_MODLUNIT NRN_FOREIGN_MPIEXEC NRN_FOREIGN_FEATURE_<NAME>      - ON/OFF for known keys
# NRN_FOREIGN_PROBE_JSON          - path to last probe output file

set(NRN_FOREIGN_PYTHON
    ""
    CACHE FILEPATH "Python interpreter that can import the foreign NEURON install (venv wheel)")
set(NRN_FOREIGN_ROOT
    ""
    CACHE PATH "Optional install prefix (bin/ prepended for discovery and tests; prefix backend)")

if(NRN_FOREIGN_PYTHON STREQUAL "")
  find_package(
    Python3
    COMPONENTS Interpreter
    REQUIRED)
  set(NRN_FOREIGN_PYTHON
      "${Python3_EXECUTABLE}"
      CACHE FILEPATH "Python interpreter that can import the foreign NEURON install (venv wheel)"
            FORCE)
endif()

if(NOT EXISTS "${NRN_FOREIGN_PYTHON}")
  message(FATAL_ERROR "NRN_FOREIGN_PYTHON does not exist: ${NRN_FOREIGN_PYTHON}")
endif()

set(_NRN_FOREIGN_PROBE "${CMAKE_CURRENT_LIST_DIR}/probe_neuron.py")
if(NOT EXISTS "${_NRN_FOREIGN_PROBE}")
  message(FATAL_ERROR "Missing probe script: ${_NRN_FOREIGN_PROBE}")
endif()

set(_NRN_FOREIGN_PROBE_OUT "${CMAKE_BINARY_DIR}/foreign_neuron_probe.json")

# Prefer scripts next to NRN_FOREIGN_PYTHON (venv bin/) and optional NRN_FOREIGN_ROOT/bin. Drop
# ambient PYTHONPATH so a developer’s *other* build-tree install cannot shadow the foreign install.
# For a classic prefix, put ${ROOT}/lib/python on PYTHONPATH (NEURON’s default
# NRN_INSTALL_PYTHON_PREFIX parent). Wheels rely on site-packages instead.
if(WIN32)
  set(NRN_FOREIGN_ENV_SEP ";")
else()
  set(NRN_FOREIGN_ENV_SEP ":")
endif()

get_filename_component(_nrn_foreign_py_bindir "${NRN_FOREIGN_PYTHON}" DIRECTORY)
set(_nrn_foreign_probe_path "${_nrn_foreign_py_bindir}")
set(_nrn_foreign_probe_pythonpath "")
if(NOT NRN_FOREIGN_ROOT STREQUAL "")
  if(NOT IS_DIRECTORY "${NRN_FOREIGN_ROOT}")
    message(FATAL_ERROR "NRN_FOREIGN_ROOT is not a directory: ${NRN_FOREIGN_ROOT}")
  endif()
  set(_nrn_foreign_probe_path
      "${NRN_FOREIGN_ROOT}/bin${NRN_FOREIGN_ENV_SEP}${_nrn_foreign_py_bindir}")
  set(_nrn_foreign_probe_pythonpath "${NRN_FOREIGN_ROOT}/lib/python")
  message(STATUS "Foreign root (prefix)     : ${NRN_FOREIGN_ROOT}")
endif()
# Optional override (native-separated), e.g. custom NRN_INSTALL_PYTHON_PREFIX parent.
set(NRN_FOREIGN_PYTHONPATH
    ""
    CACHE STRING "Optional PYTHONPATH entries for foreign discovery/tests (prepended)")
if(NOT NRN_FOREIGN_PYTHONPATH STREQUAL "")
  if(_nrn_foreign_probe_pythonpath STREQUAL "")
    set(_nrn_foreign_probe_pythonpath "${NRN_FOREIGN_PYTHONPATH}")
  else()
    set(_nrn_foreign_probe_pythonpath
        "${NRN_FOREIGN_PYTHONPATH}${NRN_FOREIGN_ENV_SEP}${_nrn_foreign_probe_pythonpath}")
  endif()
endif()
# Expose for ForeignTestHelpers / tests (FORCE so reconfigure with ROOT updates).
set(NRN_FOREIGN_PATH_PREFIX
    "${_nrn_foreign_probe_path}"
    CACHE INTERNAL "PATH prefix for foreign tools" FORCE)
set(NRN_FOREIGN_SITE_PYTHONPATH
    "${_nrn_foreign_probe_pythonpath}"
    CACHE INTERNAL "PYTHONPATH prefix for foreign neuron package (prefix installs)" FORCE)
message(STATUS "Foreign site PYTHONPATH   : ${NRN_FOREIGN_SITE_PYTHONPATH}")

execute_process(
  COMMAND
    "${CMAKE_COMMAND}" -E env "PYTHONPATH=${_nrn_foreign_probe_pythonpath}"
    "PATH=${_nrn_foreign_probe_path}${NRN_FOREIGN_ENV_SEP}$ENV{PATH}" "${NRN_FOREIGN_PYTHON}"
    "${_NRN_FOREIGN_PROBE}"
  RESULT_VARIABLE _probe_rc
  OUTPUT_VARIABLE _probe_stdout
  ERROR_VARIABLE _probe_stderr
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE)

if(NOT _probe_rc EQUAL 0)
  message(
    FATAL_ERROR
      "Foreign NEURON probe failed (rc=${_probe_rc}).\n"
      "Python: ${NRN_FOREIGN_PYTHON}\n"
      "PYTHONPATH=${_nrn_foreign_probe_pythonpath}\n"
      "stdout:\n${_probe_stdout}\n"
      "stderr:\n${_probe_stderr}\n"
      "For a wheel: pip install neuron-nightly into a venv and pass that python.\n"
      "For a prefix: ninja install first; set -DNRN_FOREIGN_ROOT=<prefix>.")
endif()

file(WRITE "${_NRN_FOREIGN_PROBE_OUT}" "${_probe_stdout}\n")
set(NRN_FOREIGN_PROBE_JSON
    "${_NRN_FOREIGN_PROBE_OUT}"
    CACHE FILEPATH "Path to foreign NEURON probe JSON" FORCE)

# CMake 3.19+ string(JSON). path is a semicolon list of members, e.g. "tools;nrniv"
function(_nrn_foreign_json_get out_var json_text)
  set(_path ${ARGN})
  string(
    JSON
    _query
    ERROR_VARIABLE
    _jerr
    GET
    "${json_text}"
    ${_path})
  if(_jerr)
    set(${out_var}
        ""
        PARENT_SCOPE)
    return()
  endif()
  if(_query STREQUAL "null")
    set(_query "")
  endif()
  set(${out_var}
      "${_query}"
      PARENT_SCOPE)
endfunction()

_nrn_foreign_json_get(NRN_FOREIGN_NEURON_VERSION "${_probe_stdout}" version)
_nrn_foreign_json_get(NRN_FOREIGN_NEURON_VERSION_FULL "${_probe_stdout}" version_full)
_nrn_foreign_json_get(NRN_FOREIGN_NEURON_GIT_SHA "${_probe_stdout}" git_sha)
_nrn_foreign_json_get(NRN_FOREIGN_NEURON_FILE "${_probe_stdout}" neuron_file)
_nrn_foreign_json_get(NRN_FOREIGN_NRNIV "${_probe_stdout}" tools nrniv)
_nrn_foreign_json_get(NRN_FOREIGN_NRNIVMODL "${_probe_stdout}" tools nrnivmodl)
_nrn_foreign_json_get(NRN_FOREIGN_MODLUNIT "${_probe_stdout}" tools modlunit)
_nrn_foreign_json_get(NRN_FOREIGN_MPIEXEC "${_probe_stdout}" tools mpiexec)

# Feature flags commonly used by NeuronTestHelper REQUIRES
set(_feature_keys
    NRN_ENABLE_PYTHON
    NRN_ENABLE_MPI
    NRN_ENABLE_MPI_DYNAMIC
    NRN_ENABLE_CORENEURON
    NRN_ENABLE_RX3D
    NRN_ENABLE_THREADS
    NRN_ENABLE_MUSIC
    CORENRN_ENABLE_GPU
    CORENRN_ENABLE_SHARED)

_nrn_foreign_json_get(_features_error "${_probe_stdout}" features _error)
if(NOT _features_error STREQUAL "")
  message(WARNING "Foreign probe could not read neuron.config.arguments: ${_features_error}\n"
                  "All NRN_FOREIGN_FEATURE_* flags default to OFF.")
endif()

set(_missing_feature_keys "")
foreach(_fk IN LISTS _feature_keys)
  _nrn_foreign_json_get(_fval "${_probe_stdout}" features "${_fk}")
  # string(JSON) maps JSON true/false to ON/OFF (CMake booleans)
  set(_on OFF)
  set(_known ON)
  if(_fval STREQUAL "")
    # Missing key (old wheel) → feature off, note it
    set(_known OFF)
    list(APPEND _missing_feature_keys "${_fk}")
  elseif(
    _fval STREQUAL "ON"
    OR _fval STREQUAL "1"
    OR _fval STREQUAL "TRUE"
    OR _fval STREQUAL "true"
    OR _fval STREQUAL "True")
    set(_on ON)
  endif()
  set(NRN_FOREIGN_FEATURE_${_fk}
      "${_on}"
      CACHE BOOL "Foreign NEURON feature ${_fk}" FORCE)
  set(NRN_FOREIGN_FEATURE_${_fk}_KNOWN
      "${_known}"
      CACHE BOOL "Whether wheel reported ${_fk}" FORCE)
endforeach()
if(NOT _missing_feature_keys STREQUAL "")
  message(STATUS "Foreign feature keys missing (treated as OFF): ${_missing_feature_keys}")
endif()

# Required tools for any useful foreign run
if(NRN_FOREIGN_NRNIVMODL STREQUAL "")
  message(
    FATAL_ERROR
      "Foreign nrnivmodl not found on PATH for ${NRN_FOREIGN_PYTHON}.\n"
      "Ensure the venv/prefix bin directory is used (NRN_FOREIGN_PYTHON / " "NRN_FOREIGN_ROOT).")
endif()
if(NRN_FOREIGN_NRNIV STREQUAL "")
  message(WARNING "Foreign nrniv not found on PATH; HOC smoke and some tests will be skipped")
endif()

# MPI tests need a launcher on the host even if the wheel was built with MPI
if(NRN_FOREIGN_FEATURE_NRN_ENABLE_MPI AND NRN_FOREIGN_MPIEXEC STREQUAL "")
  message(STATUS "Foreign NEURON has MPI, but mpiexec was not found on PATH; "
                 "MPI tests will not be registered until a launcher is available")
  set(NRN_FOREIGN_FEATURE_NRN_ENABLE_MPI
      OFF
      CACHE BOOL "Foreign NEURON feature NRN_ENABLE_MPI" FORCE)
endif()

# If CoreNEURON is on but SHARED is unknown/missing, assume shared (wheels are shared).
if(NRN_FOREIGN_FEATURE_NRN_ENABLE_CORENEURON AND NOT
                                                 NRN_FOREIGN_FEATURE_CORENRN_ENABLE_SHARED_KNOWN)
  message(STATUS "CORENRN_ENABLE_SHARED not reported by wheel; assuming ON (typical for wheels)")
  set(NRN_FOREIGN_FEATURE_CORENRN_ENABLE_SHARED
      ON
      CACHE BOOL "Foreign NEURON feature CORENRN_ENABLE_SHARED" FORCE)
endif()

message(STATUS "Foreign NEURON python     : ${NRN_FOREIGN_PYTHON}")
message(STATUS "Foreign NEURON module     : ${NRN_FOREIGN_NEURON_FILE}")
message(STATUS "Foreign NEURON version    : ${NRN_FOREIGN_NEURON_VERSION_FULL}")
message(STATUS "Foreign NEURON git sha    : ${NRN_FOREIGN_NEURON_GIT_SHA}")
message(STATUS "Foreign nrniv             : ${NRN_FOREIGN_NRNIV}")
message(STATUS "Foreign nrnivmodl         : ${NRN_FOREIGN_NRNIVMODL}")
message(STATUS "Foreign MPI               : ${NRN_FOREIGN_FEATURE_NRN_ENABLE_MPI}")
message(STATUS "Foreign MPI dynamic       : ${NRN_FOREIGN_FEATURE_NRN_ENABLE_MPI_DYNAMIC}")
message(STATUS "Foreign CoreNEURON        : ${NRN_FOREIGN_FEATURE_NRN_ENABLE_CORENEURON}")
message(STATUS "Foreign CoreNEURON shared : ${NRN_FOREIGN_FEATURE_CORENRN_ENABLE_SHARED}")
message(STATUS "Foreign GPU               : ${NRN_FOREIGN_FEATURE_CORENRN_ENABLE_GPU}")
message(STATUS "Foreign mpiexec           : ${NRN_FOREIGN_MPIEXEC}")
message(STATUS "Foreign probe JSON        : ${NRN_FOREIGN_PROBE_JSON}")
