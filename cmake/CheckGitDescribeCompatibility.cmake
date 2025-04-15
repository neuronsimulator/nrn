# CheckGitDescribeCompatibility.cmake
#
# Validates that git describe output is compatible with PEP 440 versioning. Normalizes the version
# string to a PEP 440-compliant format for validation purposes. Used to check user-chosen tags, not
# to set PyPI package versions.

find_package(Git)
if(NOT GIT_FOUND)
  message(FATAL_ERROR "Git not found. Please ensure Git is installed and accessible.")
endif()

# Execute git describe to get the version string
execute_process(
  COMMAND ${GIT_EXECUTABLE} describe --tags
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE GIT_DESCRIBE_RESULT)

if(NOT GIT_DESCRIBE_RESULT EQUAL 0)
  message(FATAL_ERROR "git describe failed: ${GIT_DESCRIBE_OUTPUT}")
endif()

# Function to normalize version string to PEP 440
function(normalize_pep440_version input_version output_version)
  # Remove leading 'v'
  string(REGEX REPLACE "^v" "" clean_version "${input_version}")

  # Match version patterns: X.Y, X.Y.Z, X.Ya, X.Y.ZaN, X.Y.ZrcN, X.Y.Z-N-gHASH
  string(REGEX MATCH "^([0-9]+\\.[0-9]+(\\.[0-9]+)?)(a|b|rc)?([0-9]+)?(-([0-9]+)-g[0-9a-f]+)?$"
               version_match "${clean_version}")
  if(NOT version_match)
    message(
      FATAL_ERROR
        "Version '${clean_version}' does not match expected format (e.g., 1.2, 1.2.3, 1.2a, 1.2.3aN, 1.2.3rcN, or 1.2.3-N-gabc123)"
    )
  endif()

  # Extract components
  set(main_version "${CMAKE_MATCH_1}")
  set(third_digit "${CMAKE_MATCH_2}")
  set(pre_type "${CMAKE_MATCH_3}")
  set(pre_num "${CMAKE_MATCH_4}")
  set(post_part "${CMAKE_MATCH_6}")

  # Normalize base version
  if(NOT third_digit)
    set(normalized_version "${main_version}.0")
  else()
    set(normalized_version "${main_version}")
  endif()

  # Normalize pre-release (a, b, rc)
  if(pre_type AND pre_num)
    set(normalized_version "${normalized_version}.${pre_type}${pre_num}")
  elseif(pre_type)
    set(normalized_version "${normalized_version}.${pre_type}0")
  endif()

  # Normalize post-release
  if(post_part)
    set(normalized_version "${normalized_version}.post${post_part}")
  endif()

  # Validate PEP 440 compliance
  if(NOT normalized_version MATCHES
     "^[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)*(\\.a[0-9]+|\\.b[0-9]+|\\.rc[0-9]+)?(\\.post[0-9]+)?$")
    message(FATAL_ERROR "Normalized version '${normalized_version}' is not PEP 440 compliant")
  endif()

  set(${output_version}
      "${normalized_version}"
      PARENT_SCOPE)
endfunction()

# Normalize the git describe output
normalize_pep440_version("${GIT_DESCRIBE_OUTPUT}" PEP440_VERSION)

# Output the normalized version
message(STATUS "PEP 440 compliant version: ${PEP440_VERSION}")

# Set the version variable for use in the project
set(PROJECT_VERSION
    "${PEP440_VERSION}"
    CACHE STRING "Project version normalized to PEP 440")
