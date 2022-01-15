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

    rm -f ${SPACK_ROOT}/etc/spack/*.yaml
    cp ${SPACK_ROOT}/bluebrain/sysconfig/bluebrain5/*.yaml ${SPACK_ROOT}/etc/spack

    # Remove configs from $HOME/.spack
    rm -rf $HOME/.spack/*
    mkdir -p $HOME/.spack

    # minimum user config for creating modules
    cat << EOF >> $HOME/.spack/modules.yaml
    modules:
      default:
        tcl:
          hash_length: 0
          whitelist: ['neuron+debug']
          projections:
            all: '{name}/{version}'
EOF

)

source ${JENKINS_DIR:-.}/_env_setup.sh

install_spack

