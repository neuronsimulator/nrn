#!/usr/bin/bash

set -e

. /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
module load intel hpe-mpi
TEST_DIR="$1"
CORENRN_TYPE="$2"
cd $WORKSPACE/${TEST_DIR}
rm -rf $WORKSPACE/${TEST_DIR}/${CORENRN_TYPE}
if [ "${TEST_DIR}" = "ringtest" ]; then
    $WORKSPACE/install_${CORENRN_TYPE}/bin/nrnivmodl-core -o ${CORENRN_TYPE} mod-ext
else
    $WORKSPACE/install_${CORENRN_TYPE}/bin/nrnivmodl-core -o ${CORENRN_TYPE} mod
fi