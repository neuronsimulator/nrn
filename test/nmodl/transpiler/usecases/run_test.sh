#! /usr/bin/env bash
set -e

nmodl="$1"
output_dir="$(uname -m)"
usecase_dir="$2"

pushd "${usecase_dir}"

# NRN + nocmodl
rm -r "${output_dir}" tmp || true
nrnivmodl
"$(uname -m)/special" -nogui simulate.py

# NRN + NMODL
rm -r "${output_dir}" tmp || true
nrnivmodl -nmodl "${nmodl}"
"$(uname -m)/special" -nogui simulate.py

popd
