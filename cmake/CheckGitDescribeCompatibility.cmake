# CheckGitDescribeCompatibility.cmake
#
# ~~~
# Sets PROJECT_VERSION
# Validates that git describe output is compatible with PEP 440 versioning.
# Normalizes the version string to a PEP 440-compliant format for validation
# purposes. Skips validation gracefully if tags are unavailable (e.g., in shallow
# clones). Used to check user-chosen tags, not to set PyPI package versions.
# ~~~

find_package(Git QUIET)
if(NOT GIT_FOUND)
  message(WARNING "Git not found. Skipping version validation.")
  set(PROJECT_VERSION
      "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
      CACHE STRING "Project version")
  message(STATUS "PROJECT_VERSION ${PROJECT_VERSION}")
  return()
endif()

# Execute git describe to get the version string
execute_process(
  COMMAND ${GIT_EXECUTABLE} describe --tags
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE GIT_EXIT_CODE
  ERROR_QUIET)

if(NOT GIT_EXIT_CODE EQUAL 0)
  message(WARNING "git describe failed (exit code: ${GIT_EXIT_CODE}). Skipping version validation.")
  set(PROJECT_VERSION
      "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
      CACHE STRING "Project version")
  message(STATUS "PROJECT_VERSION ${PROJECT_VERSION}")
  return()
endif()

# ~~~
# Function to check if a version string is canonical per PEP 440
# Usage: is_pep440_canonical_version(VERSION <version_string> RESULT <output_variable>)
# Returns TRUE if the version matches the canonical PEP 440 format, FALSE otherwise.
# Since it seems that
#   "^([1-9][0-9]*!)?([0-9]|[1-9][0-9]*)(\\.([0-9]|[1-9][0-9]*))*((a|b|rc)([0-9]|[1-9][0-9]*))?(\\.post([0-9]|[1-9][0-9]*))?(\\.dev([0-9]|[1-9][0-9]*))?$"
# is too complicated for some versions of cmake, eg. 3.22.1, we use Python to do the check.
# ~~~
function(is_pep440_canonical_version)
  cmake_parse_arguments(ARG "" "VERSION;RESULT" "" ${ARGN})
  if(NOT ARG_VERSION OR NOT ARG_RESULT)
    message(FATAL_ERROR "is_pep440_canonical_version requires VERSION and RESULT arguments")
  endif()

  message(STATUS "Validating version: ${ARG_VERSION}")

  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_SOURCE_DIR}/cmake/check_pep440_version.py"
            "${ARG_VERSION}"
    OUTPUT_VARIABLE PYTHON_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE PYTHON_EXIT_CODE
    ERROR_QUIET)

  if(NOT PYTHON_EXIT_CODE EQUAL 0)
    message(
      WARNING
        "Python validation failed for version '${ARG_VERSION}' (exit code: ${PYTHON_EXIT_CODE})")
    set(${ARG_RESULT}
        FALSE
        PARENT_SCOPE)
    return()
  endif()

  if(PYTHON_OUTPUT STREQUAL "TRUE")
    set(${ARG_RESULT}
        TRUE
        PARENT_SCOPE)
  else()
    set(${ARG_RESULT}
        FALSE
        PARENT_SCOPE)
  endif()
endfunction()

# Function to normalize version string to PEP 440
function(normalize_pep440_version input_version output_version expected_project_version)
  # Remove leading 'v'
  string(REGEX REPLACE "^v" "" clean_version "${input_version}")

  # Match version patterns: X.Y(.Z)?(\.?(a|b|rc|dev))?(N)?(-N-gHASH)?
  string(REGEX MATCH
               "^([0-9]+\\.[0-9]+(\\.[0-9]+)?)(\\.?(a|b|rc|dev))?([0-9]+)?(-([0-9]+)-g[0-9a-f]+)?$"
               version_match "${clean_version}")
  if(NOT version_match)
    message(
      WARNING
        "Version '${clean_version}' does not match expected format X.Y(.Z)?(.?(a|b|rc|dev))?(N)?(-N-gHASH)?. Note '?' means 0 or 1 of preceding group."
    )
    set(${output_version}
        ""
        PARENT_SCOPE)
    set(${expected_project_version}
        ""
        PARENT_SCOPE)
    return()
  endif()

  # Extract components
  set(main_version "${CMAKE_MATCH_1}")
  set(third_digit "${CMAKE_MATCH_2}")
  set(pre_type "${CMAKE_MATCH_4}")
  set(pre_num "${CMAKE_MATCH_5}")
  set(post_part "${CMAKE_MATCH_7}")
  message(STATUS "main_version ${main_version}")
  message(STATUS "third_digit ${third_digit}")
  message(STATUS "pre_type ${pre_type}")
  message(STATUS "pre_num ${pre_num}")
  message(STATUS "post_part ${post_part}")

  # Normalize to PEP 440
  if(NOT third_digit)
    set(normalized_version "${main_version}.0")
  else()
    set(normalized_version "${main_version}")
  endif()

  # Normalize pre-release (a, b, rc)
  if(pre_type AND NOT "${pre_type}" STREQUAL "dev")
    if(pre_num)
      set(normalized_version "${normalized_version}${pre_type}${pre_num}")
    else()
      set(normalized_version "${normalized_version}${pre_type}0")
    endif()
  endif()

  # Normalize post-release
  if(post_part)
    set(normalized_version "${normalized_version}.post${post_part}")
  endif()

  # if pre_type is dev, it goes at the end with a .
  if(pre_type AND "${pre_type}" STREQUAL "dev")
    if(pre_num)
      set(normalized_version "${normalized_version}.${pre_type}${pre_num}")
    else()
      set(normalized_version "${normalized_version}.${pre_type}0")
    endif()
  endif()

  # Derive expected project version (for CMakeLists.txt comparison)
  if(pre_type AND NOT "${pre_type}" STREQUAL "dev")
    set(expected_version "${main_version}.0")
  else()
    set(expected_version "${main_version}")
    if(NOT third_digit)
      set(expected_version "${expected_version}.0")
    endif()
  endif()

  # Validate PEP 440 compliance
  is_pep440_canonical_version(VERSION ${normalized_version} RESULT result)
  if(NOT result)
    message(
      WARNING
        "From '${input_version}' Normalized version '${normalized_version}' is not PEP 440 compliant."
    )
    set(${output_version}
        ""
        PARENT_SCOPE)
    set(${expected_project_version}
        ""
        PARENT_SCOPE)
    return()
  endif()

  set(${output_version}
      "${normalized_version}"
      PARENT_SCOPE)
  set(${expected_project_version}
      "${expected_version}"
      PARENT_SCOPE)
endfunction()

# Normalize the git describe output
normalize_pep440_version("${GIT_DESCRIBE_OUTPUT}" PEP440_VERSION EXPECTED_PROJECT_VERSION)

if(PEP440_VERSION)
  message(STATUS "From '${GIT_DESCRIBE_OUTPUT}', PEP 440 compliant version: ${PEP440_VERSION}")
  # Compare with PROJECT_VERSION
  set(cmake_version "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
  if(EXPECTED_PROJECT_VERSION AND NOT cmake_version STREQUAL EXPECTED_PROJECT_VERSION)
    message(
      WARNING
        "Git tag suggests project version '${EXPECTED_PROJECT_VERSION}' but CMakeLists.txt specifies '${cmake_version}'. Consider updating CMakeLists.txt."
    )
  endif()
else()
  message(WARNING "Unable to validate git describe output.")
  set(PEP440_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

# Set the version variable for use in the project
set(PROJECT_VERSION
    "${PEP440_VERSION}"
    CACHE STRING "Project version normalized to PEP 440" FORCE)
message(STATUS "PROJECT_VERSION ${PROJECT_VERSION}")
