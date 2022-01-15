#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh

set -x
CORENRN_TYPE="$1"
export MPI_UNBUFFERED_STDIO=1

# load boost to avoid runtime error
module load unstable boost

if [ "${CORENRN_TYPE}" = "non-gpu" ]; then
    module load intel-oneapi-compilers
fi

cd $WORKSPACE/build_${CORENRN_TYPE}
echo "Testing ${CORENRN_TYPE}"
ctest --output-on-failure -T test --no-compress-output -E cppcheck_test

# unload
module unload unstable boost
