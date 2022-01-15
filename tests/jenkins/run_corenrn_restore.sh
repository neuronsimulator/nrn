#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh

set -x
CORENRN_TYPE="$1"
MPI_RANKS="$2"

if [ "${CORENRN_TYPE}" = "non-gpu" ]; then
    module load intel-oneapi-compilers
fi

cd $WORKSPACE/ringtest

mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 10 -d coredat --checkpoint part0 --outpath part0
mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 40 -d coredat --checkpoint part1 --restore part0 --outpath part1
mpirun -n ${MPI_RANKS} ./${CORENRN_TYPE}/special-core --mpi -e 100 -d coredat  --checkpoint part2 --restore part1 --outpath part2

cat part0/out.dat > ringtest/cnrn.dat
cat part1/out.dat >> ringtest/cnrn.dat
cat part2/out.dat >> ringtest/cnrn.dat

cat ringtest/cnrn.dat | sort -k 1n,1n -k 2n,2n > ringtest/out_cn_restore_ringtest.spk
diff -w -q ringtest/out_nrn_ringtest.spk ringtest/out_cn_restore_ringtest.spk
