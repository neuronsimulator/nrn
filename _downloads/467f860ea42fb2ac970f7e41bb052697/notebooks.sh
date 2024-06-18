#!/usr/bin/env bash
set -e

notebook_dirs=(
  "tutorials"
  "rxd-tutorials"
  "nmodl"
)

convert_notebooks() {
  set -e
  working_dir=$1
  echo "Running convert_notebooks in $1"
  (cd "$working_dir" && jupyter nbconvert --debug --to notebook --inplace --execute *.ipynb)
}

clean_notebooks() {
  set -e
  working_dir=$1
  echo "Running clean_notebooks in $1"
  (cd "$working_dir" && jupyter nbconvert --ClearOutputPreprocessor.enabled=True --ClearMetadataPreprocessor.enabled=True --clear-output --inplace *.ipynb)
}

if [ $# -ge 1 ]; then
    echo "Cleaning notebooks output."
    for i in ${notebook_dirs[@]} ; do
      clean_notebooks "$i"
    done
else
  echo "Executing and embedding outputs inplace into jupyter notebooks."
  for i in ${notebook_dirs[@]} ; do
    convert_notebooks "$i"
  done
  echo 'Done. NOTE: remember to run target `notebooks-clean` before a PR. The `docs` target does this automatically.'
fi


