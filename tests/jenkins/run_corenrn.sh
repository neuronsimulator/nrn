#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh

set -x
TEST_DIR="$1"
CORENRN_TYPE="$2"
TEST="$3"
MPI_RANKS="$4"

cd $WORKSPACE/${TEST_DIR}

if [ "${TEST_DIR}" = "testcorenrn" ] || [ "${CORENRN_TYPE}" = "AoS" ]; then
    export OMP_NUM_THREADS=1
else
    export OMP_NUM_THREADS=2
fi

if [ "${TEST}" = "patstim" ]; then
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 --pattern patstim.spk -d test${TEST}dat -o ${TEST}
elif [ "${TEST}" = "ringtest" ]; then
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 -d coredat -o ${TEST}
else
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 -d test${TEST}dat -o ${TEST}
fi

mv ${TEST}/out.dat ${TEST}/out_cn_${TEST}.spk
diff -w ${TEST}/out_nrn_${TEST}.spk ${TEST}/out_cn_${TEST}.spk
