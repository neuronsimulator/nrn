#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh
module load neuron/develop intel

set -x
CORENRN_TYPE="$1"
export PATH=$WORKSPACE/install_${CORENRN_TYPE}/bin:$PATH

# temporary build directory
build_dir=$(mktemp -d $(pwd)/build_XXXX)
cd $build_dir

# build special and special-core
nrnivmodl ../tests/jenkins/mod
nrnivmodl-core ../tests/jenkins/mod
ls -la x86_64

# run test sim with external mechanism
python $WORKSPACE/tests/jenkins/neuron_direct.py

# remove build directory
cd -
rm -rf $build_dir
