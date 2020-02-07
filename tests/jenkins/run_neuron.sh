#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh

set -x
TEST_DIR="$1"
TEST="$2"
MPI_RANKS="$3"

cd $WORKSPACE/${TEST_DIR}

if [ "${TEST_DIR}" = "testcorenrn" ]; then
    mkdir test${TEST}dat
    mkdir ${TEST}
    mpirun -n ${MPI_RANKS} ./x86_64/special -mpi -c sim_time=100 test${TEST}.hoc
    cat out${TEST}.dat | sort -k 1n,1n -k 2n,2n > ${TEST}/out_nrn_${TEST}.spk
    rm out${TEST}.dat
elif [ "${TEST_DIR}" = "ringtest" ]; then
    mkdir ${TEST}
    mpirun -n 6 ./x86_64/special ringtest.py -mpi
    cat coredat/spk6.std | sort -k 1n,1n -k 2n,2n > ${TEST}/out_nrn_${TEST}.spk
else
    echo "Not a valid TEST"
    exit 1
fi