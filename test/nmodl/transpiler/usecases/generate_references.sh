#! /usr/bin/env bash
set -eu

if [[ $# -ne 3 ]]
then
  echo "Usage: $0 NMODL USECASE_DIR OUTPUT_DIR"
  exit -1
fi

nmodl="$1"
usecase_dir="$2"
output_dir="$3"

function sanitize() {
  for f in "${1}"/*.cpp
  do
    if [[ "$(uname)" == 'Darwin' ]]; then
        sed_cmd=("sed" "-i" "")
    else
        sed_cmd=("sed" "-i")
    fi
    "${sed_cmd[@]}" "s/Created         : .*$/Created         : DATE/" "$f"
    "${sed_cmd[@]}" "s/NMODL Compiler  : .*$/NMODL Compiler  : VERSION/" "$f"
  done
}

"${nmodl}" "${usecase_dir}"/*.mod --neuron -o "${output_dir}/neuron"
sanitize "${output_dir}/neuron"

"${nmodl}" "${usecase_dir}"/*.mod -o "${output_dir}"/coreneuron
sanitize "${output_dir}/coreneuron"
