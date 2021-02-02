#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh
module purge
module load unstable neuron/develop intel hpe-mpi

set -x
CORENRN_TYPE="$1"
export PATH=$WORKSPACE/install_${CORENRN_TYPE}/bin:$PATH

# temporary build directory
build_dir=$(mktemp -d $(pwd)/build_XXXX)
cd $build_dir

# build special and special-core
nrnivmodl-core ../tests/jenkins/mod
nrnivmodl ../tests/jenkins/mod
ls -la x86_64

# Unload intel module to avoid issue whith mpirun
module unload intel

# run test sim with external mechanism
mpirun -n 1 ./x86_64/special -python -mpi $WORKSPACE/tests/jenkins/neuron_direct.py | grep "Voltage times and i_membrane_ are same and difference is less than 1e-10"

# remove build directory
cd -
rm -rf $build_dir
