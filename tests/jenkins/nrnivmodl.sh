#!/usr/bin/bash

set -e
source ${JENKINS_DIR:-.}/_env_setup.sh
module load neuron

set -x
TEST_DIR="$1"

cd $WORKSPACE/${TEST_DIR}
nrnivmodl mod
