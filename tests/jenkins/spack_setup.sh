#!/bin/bash

# Based on spack_setup.sh from blueconfigs repo

echo "
=====================================================================
Preparing spack environment...
====================================================================="

export SPACK_INSTALL_PREFIX="${SPACK_INSTALL_PREFIX:-${WORKSPACE}/INSTALL_HOME}"
export SOFTS_DIR_PATH=$SPACK_INSTALL_PREFIX  # Deprecated, but might still be reqd

BUILD_HOME="${WORKSPACE}/BUILD_HOME"
export SPACK_ROOT="${BUILD_HOME}/spack"

# ENV SETUP

# TODO: /usr/bin was added as a quickfix due to git dependencies probs
export PATH=$SPACK_ROOT/bin/spack:/usr/bin:$PATH

# MODULES
# Use spack only modules. Last one is added by changing MODULEPATH since it might not exist yet
module purge
unset MODULEPATH
source /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
export MODULEPATH=$SPACK_INSTALL_PREFIX/modules/tcl/$(spack arch):$MODULEPATH

############################# CLONE/SETUP REPOSITORY #############################

install_spack() (
    set -e
    BASEDIR="$(dirname "$SPACK_ROOT")"
    mkdir -p $BASEDIR && cd $BASEDIR
    rm -rf .spack   # CLEANUP SPACK CONFIGS
    SPACK_REPO=https://github.com/BlueBrain/spack.git
    SPACK_BRANCH=${SPACK_BRANCH:-"develop"}

    echo "Installing SPACK. Cloning $SPACK_REPO $SPACK_ROOT --depth 1 -b $SPACK_BRANCH"
    git clone $SPACK_REPO $SPACK_ROOT --depth 1 -b $SPACK_BRANCH
    # Use BBP configs
    mkdir -p $SPACK_ROOT/etc/spack/defaults/linux
    cp /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/*.yaml $SPACK_ROOT/etc/spack/

)


install_spack

