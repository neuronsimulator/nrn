# Version policy D for foreign ctest: - NRN_FOREIGN_CI=ON            -> mismatch is FATAL -
# otherwise mismatch is FATAL unless NRN_FOREIGN_ALLOW_SKEW=ON Match prefers git SHA (flexible
# length / embedded in describe), then version string.

option(NRN_FOREIGN_CI "Treat foreign/source version mismatch as fatal (CI mode)" OFF)
option(NRN_FOREIGN_ALLOW_SKEW
       "Allow foreign wheel / source tree version mismatch (local exploration)" OFF)

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

# ---------------------------------------------------------------------------
# Normalize / extract git SHAs
# ---------------------------------------------------------------------------
# Pull a hex SHA from values like "5ac449d89", "g5ac449d89", "9.0.1-85-g5ac449d89".
function(_nrn_foreign_extract_git_sha out_var text)
  set(_t "${text}")
  if(_t STREQUAL "" OR _t STREQUAL "null")
    set(${out_var}
        ""
        PARENT_SCOPE)
    return()
  endif()
  string(TOLOWER "${_t}" _t)
  # Prefer -g<sha> / g<sha> from git describe style strings
  if(_t MATCHES "g([0-9a-f]+)")
    set(_sha "${CMAKE_MATCH_1}")
  elseif(_t MATCHES "^([0-9a-f]+)$")
    set(_sha "${CMAKE_MATCH_1}")
  else()
    set(_sha "")
  endif()
  set(${out_var}
      "${_sha}"
      PARENT_SCOPE)
endfunction()

# True if either SHA is a prefix of the other (require at least 7 hex chars).
function(_nrn_foreign_sha_match out_var a b)
  set(${out_var}
      OFF
      PARENT_SCOPE)
  if(a STREQUAL "" OR b STREQUAL "")
    return()
  endif()
  string(LENGTH "${a}" _la)
  string(LENGTH "${b}" _lb)
  if(_la LESS 7 OR _lb LESS 7)
    return()
  endif()
  if(a STREQUAL b)
    set(${out_var}
        ON
        PARENT_SCOPE)
    return()
  endif()
  if(_la GREATER_EQUAL _lb)
    string(SUBSTRING "${a}" 0 ${_lb} _ap)
    if(_ap STREQUAL "${b}")
      set(${out_var}
          ON
          PARENT_SCOPE)
    endif()
  else()
    string(SUBSTRING "${b}" 0 ${_la} _bp)
    if(_bp STREQUAL "${a}")
      set(${out_var}
          ON
          PARENT_SCOPE)
    endif()
  endif()
endfunction()

_nrn_foreign_extract_git_sha(_wheel_sha_raw "${NRN_FOREIGN_NEURON_GIT_SHA}")
if(_wheel_sha_raw STREQUAL "")
  _nrn_foreign_extract_git_sha(_wheel_sha_raw "${NRN_FOREIGN_NEURON_VERSION_FULL}")
endif()
if(_wheel_sha_raw STREQUAL "")
  _nrn_foreign_extract_git_sha(_wheel_sha_raw "${NRN_FOREIGN_NEURON_VERSION}")
endif()
_nrn_foreign_extract_git_sha(_src_sha_raw "${NRN_FOREIGN_SOURCE_GIT_SHA}")
if(_src_sha_raw STREQUAL "")
  _nrn_foreign_extract_git_sha(_src_sha_raw "${NRN_FOREIGN_SOURCE_DESCRIBE}")
endif()

set(_match OFF)
set(_match_reason "no match")

# 1) Git SHA (flexible length; works for nightlies that embed g<sha> in describe)
_nrn_foreign_sha_match(_sha_ok "${_wheel_sha_raw}" "${_src_sha_raw}")
if(_sha_ok)
  set(_match ON)
  set(_match_reason "git SHA (${_wheel_sha_raw} ~ ${_src_sha_raw})")
endif()

# 2) Exact version / describe string
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

# 3) Nightly-friendly: same leading X.Y.Z and same embedded g-sha even if "commits since tag"
# counters differ (e.g. wheel rebuilt vs local describe). Already covered by (1) when SHAs extract;
# keep (2) for tagged releases.

set(NRN_FOREIGN_VERSION_MATCH
    "${_match}"
    CACHE BOOL "Whether foreign NEURON matches this source tree" FORCE)
set(NRN_FOREIGN_VERSION_MATCH_REASON
    "${_match_reason}"
    CACHE STRING "How foreign/source versions were compared" FORCE)
set(NRN_FOREIGN_WHEEL_GIT_SHA_NORMALIZED
    "${_wheel_sha_raw}"
    CACHE STRING "Normalized wheel git SHA used for matching" FORCE)
set(NRN_FOREIGN_SOURCE_GIT_SHA_NORMALIZED
    "${_src_sha_raw}"
    CACHE STRING "Normalized source git SHA used for matching" FORCE)

message(STATUS "Source tree describe      : ${NRN_FOREIGN_SOURCE_DESCRIBE}")
message(STATUS "Source tree git sha       : ${NRN_FOREIGN_SOURCE_GIT_SHA_SHORT}")
message(STATUS "Normalized wheel sha      : ${_wheel_sha_raw}")
message(STATUS "Normalized source sha     : ${_src_sha_raw}")
message(STATUS "Foreign/source match      : ${NRN_FOREIGN_VERSION_MATCH} (${_match_reason})")

if(NOT _match)
  string(
    CONCAT
      _msg
      "Foreign NEURON does not match this source tree.\n"
      "  wheel version : ${NRN_FOREIGN_NEURON_VERSION_FULL} (sha ${NRN_FOREIGN_NEURON_GIT_SHA}"
      " / normalized ${_wheel_sha_raw})\n"
      "  source tree   : ${NRN_FOREIGN_SOURCE_DESCRIBE} (sha ${NRN_FOREIGN_SOURCE_GIT_SHA_SHORT}"
      " / normalized ${_src_sha_raw})\n"
      "  match reason  : ${_match_reason}\n")
  if(NRN_FOREIGN_CI)
    message(FATAL_ERROR "${_msg}NRN_FOREIGN_CI=ON: refusing to configure (hard match).")
  elseif(NRN_FOREIGN_ALLOW_SKEW)
    message(WARNING "${_msg}NRN_FOREIGN_ALLOW_SKEW=ON: continuing despite skew.\n"
                    "Feature gates still come from the wheel; API mismatches may fail tests.")
  else()
    message(
      FATAL_ERROR
        "${_msg}Refusing to configure. For local exploration re-run with:\n"
        "  -DNRN_FOREIGN_ALLOW_SKEW=ON\n"
        "Or check out the revision that built the wheel, or install a matching wheel.")
  endif()
endif()
