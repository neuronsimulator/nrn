#! /usr/bin/env bash
set -u

script_dir="$(dirname "$(realpath "$0")")"
nmodl="$1"
usecase_dir="$2"
references_dir="${script_dir}/references/$(basename "$2")"
output_dir="$(mktemp -d)"

"${script_dir}"/generate_references.sh ${nmodl} "${usecase_dir}" "${output_dir}"

diff -U 8 -r "${references_dir}" "${output_dir}"

exit_code=$?

rm -r "${output_dir}"

exit $exit_code
