#! /usr/bin/env bash
set -eu

function run_tests() {
  echo ""
  echo "Running tests:"
  for f in "${2}/"test_*.py "${2}/simulate.py"
  do
    if [[ -f "$f" ]]
    then
      echo "${usecase_dir}/$f: started."
      python "$f" $1
      echo "${usecase_dir}/$f: success."
    fi
  done
  echo "All tests were successful."
}

if [[ $# -ne 2 ]]
then
  echo "Usage: $0 NMODL USECASE_DIR"
  exit -1
fi

nmodl="$1"
output_dir="$(uname -m)"
usecase_dir="$2"
usecase_name="$(basename "$usecase_dir")"


# NRN + nocmodl
if [[ "matexp" != *"$usecase_name"* ]]; then # Skip matexp, not implemented in nocmodl
  echo "-- Running NRN+nocmodl ------"
  rm -r "${output_dir}" tmp || true
  nrnivmodl "${usecase_dir}"
  run_tests nocmodl "${usecase_dir}"
fi


# NRN + NMODL
echo "-- Running NRN+NMODL --------"
rm -r "${output_dir}" tmp || true
nrnivmodl -nmodl "${nmodl}" -nmodlflags "codegen --cvode" "${usecase_dir}"
run_tests nmodl "${usecase_dir}"
