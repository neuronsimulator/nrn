#!/usr/bin/bash

set -e
set -x

CORENRN_TYPE="$1"

unset MODULEPATH
. /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
export SPACK_INSTALL_PREFIX="${SPACK_INSTALL_PREFIX:-${WORKSPACE}/INSTALL_HOME}"
export PATH=$WORKSPACE/BUILD_HOME/spack/bin:/usr/bin:$PATH
export MODULEPATH=$SPACK_INSTALL_PREFIX/modules/tcl/$(spack arch):$MODULEPATH

module load hpe-mpi neuron

export CORENEURONLIB=$WORKSPACE/install_${CORENRN_TYPE}/lib/libcoreneuron.so

unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')

python $WORKSPACE/tests/jenkins/neuron_direct.py
