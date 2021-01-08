#!/bin/bash

# Based on spack_setup.sh from blueconfigs repo

echo "
=====================================================================
Preparing spack environment...
====================================================================="


############################# CLONE/SETUP REPOSITORY #############################

install_spack() (
    set -e
    BASEDIR="$(dirname "$SPACK_ROOT")"
    mkdir -p $BASEDIR && cd $BASEDIR
    rm -rf .spack   # CLEANUP SPACK CONFIGS
    SPACK_REPO=https://github.com/BlueBrain/spack.git

    # cloning branch option
    BRANCH_OPT=""
    if [ -n "$SPACK_BRANCH" ]; then
        BRANCH_OPT="-b ${SPACK_BRANCH}"
    fi

    echo "Installing SPACK. Cloning $SPACK_REPO $SPACK_ROOT --depth 1 $BRANCH_OPT"
    git clone $SPACK_REPO $SPACK_ROOT --depth 1 $BRANCH_OPT

    # Use BBP configs
    mkdir -p $SPACK_ROOT/etc/spack/defaults/linux
    cp /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/*.yaml $SPACK_ROOT/etc/spack/
    sed -i -e  's/neuron+mpi~debug%intel/neuron+mpi/g' $SPACK_ROOT/etc/spack/modules.yaml
    # Remove configs from $HOME/.spack
    rm -rf $HOME/.spack
)

source ${JENKINS_DIR:-.}/_env_setup.sh

install_spack

