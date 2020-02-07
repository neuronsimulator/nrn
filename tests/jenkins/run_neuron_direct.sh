#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh
module load neuron

set -x
CORENRN_TYPE="$1"
export CORENEURONLIB=$WORKSPACE/install_${CORENRN_TYPE}/lib/libcoreneuron.so
python $WORKSPACE/tests/jenkins/neuron_direct.py
