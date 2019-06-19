#!/usr/bin/bash

set -e
set +x

TEST_DIR="$1"

. /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
module load intel

neuron_version=$(module av neuron 2>&1 | grep -o -m 1 '^neuron.*/parallel$' | awk -F' ' '{print $1}')
if [[ $neuron_version ]]; then
    module load $neuron_version
    module list
else
    echo "Error: no compatible neuron version found." >&2
    module list
    exit 1
fi

unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')

cd $WORKSPACE/${TEST_DIR}
nrnivmodl mod
