#! /usr/bin/env bash
set -eu

nmodl="$1"
usecase_dir="$2"

if [[ $# -eq 3 ]]
then
  output_dir="$3"
else
  script_dir="$(dirname "$(realpath "$0")")"
  output_dir="${script_dir}/references/$(basename "$2")"
fi

function sanitize() {
  for f in "${1}"/*.cpp
  do
    if [[ "$(uname)" == 'Darwin' ]]
    then
        sed_cmd="sed -i''"
    else
        sed_cmd="sed -i"
    fi
    ${sed_cmd} "s/Created         : .*$/Created         : DATE/" "$f"
    ${sed_cmd} "s/NMODL Compiler  : .*$/NMODL Compiler  : VERSION/" "$f"
  done
}

"${nmodl}" "${usecase_dir}"/*.mod --neuron -o "${output_dir}/neuron"
sanitize "${output_dir}/neuron"

"${nmodl}" "${usecase_dir}"/*.mod -o "${output_dir}"/coreneuron
sanitize "${output_dir}/coreneuron"
