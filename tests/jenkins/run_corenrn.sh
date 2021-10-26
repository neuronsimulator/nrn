#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh

set -x
TEST_DIR="$1"
CORENRN_TYPE="$2"
TEST="$3"
MPI_RANKS="$4"

cd $WORKSPACE/${TEST_DIR}

if [ "${TEST_DIR}" = "testcorenrn" ] ; then
    export OMP_NUM_THREADS=1
else
    export OMP_NUM_THREADS=2
fi

if [ "${TEST}" = "patstim" ]; then
    # first run full run
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 --pattern patstim.spk -d testpatstimdat -o ${TEST}

    # split patternstim file into two parts : total 2000 events, split at line no 1001 i.e. before events for 50 msec
    echo 1000 > patstim.1.spk
    sed -n 2,1001p patstim.spk  >> patstim.1.spk
    echo 1000 > patstim.2.spk
    sed -n 1002,2001p patstim.spk  >> patstim.2.spk

    # run test with checkpoint : part 1
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 49 --pattern patstim.1.spk -d testpatstimdat -o ${TEST}_part1 --checkpoint checkpoint

    # run test with restore : part 2
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 --pattern patstim.2.spk -d testpatstimdat -o ${TEST}_part2 --restore checkpoint

    # run additional restore by providing full patternstim in part 2 : part 3
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 --pattern patstim.spk -d testpatstimdat -o ${TEST}_part3 --restore checkpoint

    # part 2 and part 3 should be same (part 3 ignore extra events in pattern stim)
    diff -w  ${TEST}_part2/out.dat  ${TEST}_part3/out.dat

    # combine spikes from part1 and part2 should be same as full run
    cat ${TEST}_part1/out.dat  ${TEST}_part2/out.dat > ${TEST}/out.saverestore.dat
    diff -w ${TEST}/out.dat ${TEST}/out.saverestore.dat

elif [ "${TEST}" = "ringtest" ]; then
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 -d coredat -o ${TEST}
elif [ "${TEST}" = "tqperf" ]; then
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 50 -d coredat --multisend -o ${TEST}
else
    mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 -d test${TEST}dat -o ${TEST}
fi

mv ${TEST}/out.dat ${TEST}/out_cn_${TEST}.spk
diff -w ${TEST}/out_nrn_${TEST}.spk ${TEST}/out_cn_${TEST}.spk
