#!/usr/bin/bash

set -e
set +x

TEST_DIR="$1"

. /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
module load neuron/2018-10/python3/parallel intel

unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')

cd $WORKSPACE/${TEST_DIR}
nrnivmodl mod
