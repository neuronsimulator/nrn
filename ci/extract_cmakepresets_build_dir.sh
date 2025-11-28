#!/usr/bin/env bash

# script for extracting build dir of a given CMake preset from CMakePresets.json
# Example:
# $ this_script.sh CMakePresets.json coverage
# build/coverage

set -euo pipefail

allowed_filenames=( "CMakePresets.json" "CMakeUserPresets.json")

usage="USAGE: $0 [CMakePresets.json|CMakeUserPresets.json] [preset]"

check_filename() {
    input="$(basename "${1:-}")"
    for filename in "${allowed_filenames[@]}"; do
        if [ "${input}" == "${filename}" ]; then
            return 0
        fi
    done
    return 1
}

parse_binary_dir() {
    file="${1:-}"
    preset="${2:-}"
    if [ -z "${file}" ] || [ -z "${preset}" ]; then
        echo "${usage}"
        exit 1
    fi
    if ! check_filename "${file}"; then
        echo "ERROR: file ${file} is not one of: ${allowed_filenames[*]}"
        exit 1
    fi
    jq -r "
      (.configurePresets[]
       | select(.name == \"${preset}\")
       | .binaryDir
       | sub(\"^\\\\$\\\\{sourceDir\\\\}/\"; \"\"))
      // error(\"binaryDir not found\")
    " "${file}"
}

parse_binary_dir "$@"
