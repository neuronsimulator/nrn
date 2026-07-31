# Version policy D for foreign ctest:
#   - NRN_FOREIGN_CI=ON            -> mismatch is FATAL
#   - otherwise mismatch is FATAL unless NRN_FOREIGN_ALLOW_SKEW=ON
# Match prefers git SHA (wheel nrnversion(3) vs source tree HEAD), then version string.

option(NRN_FOREIGN_CI "Treat foreign/source version mismatch as fatal (CI mode)" OFF)
option(NRN_FOREIGN_ALLOW_SKEW
       "Allow foreign wheel / source tree version mismatch (local exploration)" OFF)

# Source tree identity (test/foreign is inside the nrn repo: ../../ from here is repo root
# when included from test/foreign/CMakeLists.txt; use PROJECT_SOURCE_DIR after project()).
if(NOT DEFINED NRN_FOREIGN_SOURCE_ROOT)
  get_filename_component(NRN_FOREIGN_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

set(NRN_FOREIGN_SOURCE_GIT_SHA "")
set(NRN_FOREIGN_SOURCE_GIT_SHA_SHORT "")
set(NRN_FOREIGN_SOURCE_DESCRIBE "")

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${NRN_FOREIGN_SOURCE_ROOT}/.git")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${NRN_FOREIGN_SOURCE_ROOT}" rev-parse HEAD
    OUTPUT_VARIABLE NRN_FOREIGN_SOURCE_GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${NRN_FOREIGN_SOURCE_ROOT}" rev-parse --short=12 HEAD
    OUTPUT_VARIABLE NRN_FOREIGN_SOURCE_GIT_SHA_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${NRN_FOREIGN_SOURCE_ROOT}" describe --tags --long --always
    OUTPUT_VARIABLE NRN_FOREIGN_SOURCE_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()

set(_match OFF)
set(_match_reason "no match")

# 1) Git SHA: wheel short SHA is a prefix of tree HEAD (or equal)
if(NOT NRN_FOREIGN_NEURON_GIT_SHA STREQUAL "" AND NOT NRN_FOREIGN_SOURCE_GIT_SHA STREQUAL "")
  string(TOLOWER "${NRN_FOREIGN_NEURON_GIT_SHA}" _wsha)
  string(TOLOWER "${NRN_FOREIGN_SOURCE_GIT_SHA}" _ssha)
  string(TOLOWER "${NRN_FOREIGN_SOURCE_GIT_SHA_SHORT}" _ssha_short)
  if(_ssha MATCHES "^${_wsha}" OR _wsha MATCHES "^${_ssha_short}" OR _wsha STREQUAL _ssha_short)
    set(_match ON)
    set(_match_reason "git SHA (${NRN_FOREIGN_NEURON_GIT_SHA} ~ ${NRN_FOREIGN_SOURCE_GIT_SHA_SHORT})")
  endif()
endif()

# 2) Version string equality (neuron.__version__ / nrnversion(5) vs git describe)
if(NOT _match)
  set(_wv "${NRN_FOREIGN_NEURON_VERSION_FULL}")
  if(_wv STREQUAL "")
    set(_wv "${NRN_FOREIGN_NEURON_VERSION}")
  endif()
  if(NOT _wv STREQUAL "" AND NOT NRN_FOREIGN_SOURCE_DESCRIBE STREQUAL "")
    if(_wv STREQUAL "${NRN_FOREIGN_SOURCE_DESCRIBE}")
      set(_match ON)
      set(_match_reason "version string (${_wv})")
    endif()
  endif()
endif()

set(NRN_FOREIGN_VERSION_MATCH
    "${_match}"
    CACHE BOOL "Whether foreign NEURON matches this source tree" FORCE)
set(NRN_FOREIGN_VERSION_MATCH_REASON
    "${_match_reason}"
    CACHE STRING "How foreign/source versions were compared" FORCE)

message(STATUS "Source tree describe      : ${NRN_FOREIGN_SOURCE_DESCRIBE}")
message(STATUS "Source tree git sha       : ${NRN_FOREIGN_SOURCE_GIT_SHA_SHORT}")
message(STATUS "Foreign/source match      : ${NRN_FOREIGN_VERSION_MATCH} (${_match_reason})")

if(NOT _match)
  string(
    CONCAT _msg
    "Foreign NEURON does not match this source tree.\n"
    "  wheel version : ${NRN_FOREIGN_NEURON_VERSION_FULL} (sha ${NRN_FOREIGN_NEURON_GIT_SHA})\n"
    "  source tree   : ${NRN_FOREIGN_SOURCE_DESCRIBE} (sha ${NRN_FOREIGN_SOURCE_GIT_SHA_SHORT})\n"
    "  match reason  : ${_match_reason}\n")
  if(NRN_FOREIGN_CI)
    message(FATAL_ERROR "${_msg}NRN_FOREIGN_CI=ON: refusing to configure (hard match).")
  elseif(NRN_FOREIGN_ALLOW_SKEW)
    message(
      WARNING
      "${_msg}NRN_FOREIGN_ALLOW_SKEW=ON: continuing despite skew.\n"
      "Feature gates still come from the wheel; API mismatches may fail tests.")
  else()
    message(
      FATAL_ERROR
      "${_msg}Refusing to configure. For local exploration re-run with:\n"
      "  -DNRN_FOREIGN_ALLOW_SKEW=ON\n"
      "Or check out the revision that built the wheel, or install a matching wheel.")
  endif()
endif()
