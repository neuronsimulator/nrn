#!/usr/bin/env bash

# This script assumes that you have the prerequisites already installed
# See https://nrn.readthedocs.io/en/latest/install/python_wheels.html#installing-macos-prerequisites
#
# If you want the script to check the nrniv output (for version string checking), set the following environment variable
# Note that this will require manual intervention during script execution
#
# export INTERACTIVE_OK=yes

set -x

export BREW_PREFIX=$(brew --prefix)
export PATH=/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/flex/bin:$PATH

export NRN_RELEASE_UPLOAD=false
export NRN_NIGHTLY_UPLOAD=false
export NEURON_NIGHTLY_TAG=""

export SKIP_EMBEDED_PYTHON_TEST=true

if [ -n $1 ]
then
    # example: 9.0a
    export SETUPTOOLS_SCM_PRETEND_VERSION=$1
fi

set -e

packaging/python/build_wheels.bash osx 3.9 coreneuron &>3.9-output
packaging/python/build_wheels.bash osx 3.10 coreneuron &>3.10-output
packaging/python/build_wheels.bash osx 3.11 coreneuron &>3.11-output

if [ -n $INTERACTIVE_OK ]
then
    for py in 9 10 11
    do
        python3.${py} -m venv venv3.${py}
        venv3.${py}/bin/pip install wheelhouse/NEURON-${1}-cp3${py}-cp3${py}-macosx_11_0_arm64.whl
        venv3.${py}/bin/nrniv
    done
fi

bash packaging/python/test_wheels.sh python3.9 wheelhouse/NEURON-*-cp39*.whl true &>3.9-test-output
bash packaging/python/test_wheels.sh python3.10 wheelhouse/NEURON-*-cp310*.whl true &>3.10-test-output
bash packaging/python/test_wheels.sh python3.11 wheelhouse/NEURON-*-cp311*.whl true &>3.11-test-output
