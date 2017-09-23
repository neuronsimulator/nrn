#!/bin/bash

set -e
set -x

MPI_LIB_NAME="$1"

case "$TRAVIS_OS_NAME" in
    osx)
        brew install flex bison modules
        brew install python3

        case "$MPI_LIB_NAME" in
            mpich|mpich3)
                brew install mpich
                ;;
            openmpi)
                brew install openmpi
                ;;
            *)
                echo "ERROR: Unknown MPI Implementation: $MPI_LIB_NAME"
                exit 1
                ;;
        esac
    ;;

    linux)
        sudo apt-get update -q
        sudo apt-get install -y libgsl0-dev
        sudo apt-get install -y libpython3-dev

        # TODO: workaround for bug in Ubuntu 14.04
        # check https://bugs.launchpad.net/ubuntu/+source/python2.7/+bug/1115466
        sudo ln -s /usr/lib/python2.7/plat-*/_sysconfigdata_nd.py /usr/lib/python2.7/

        case "$MPI_LIB_NAME" in
            mpich|mpich3)
                sudo apt-get install -y mpich libmpich-dev
                ;;
            openmpi)
                sudo apt-get install -y openmpi-bin libopenmpi-dev
                ;;
            *)
                echo "ERROR: Unknown MPI Implementation: $MPI_LIB_NAME"
                exit 1
                ;;
        esac
        ;;

    *)
        echo "Unknown Operating System: $os"
        exit 1
        ;;
esac
