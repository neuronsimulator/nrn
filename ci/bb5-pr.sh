#!/bin/bash

set -xe

git show HEAD

source /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
module load archive/2021-08 cmake bison flex python-dev doxygen llvm ninja
module list

function bb5_pr_setup_virtualenv() {
    # latest version of breathe from 21st April has issue with 4.13.0, see https://github.com/michaeljones/breathe/issues/431
    # temporary workaround
    virtualenv venv
    . venv/bin/activate
    pip3 install "breathe<=4.12.0"
}

function build_with() {
    compiler="$1"
    module load $compiler
    . venv/bin/activate

    echo "Building NMODL with $compiler"
    module load $compiler
    rm -rf build_$compiler
    mkdir build_$compiler
    pushd build_$compiler
    cmake .. -G Ninja \
             -DCMAKE_C_COMPILER=$MPICC_CC \
             -DCMAKE_CXX_COMPILER=$MPICXX_CXX \
             -DPYTHON_EXECUTABLE=$(which python3) \
             -DNMODL_FORMATTING:BOOL=ON
    cmake --build . --parallel 6
    popd
}

function test_with() {
    compiler="$1"
    module load $compiler
    . venv/bin/activate
    cd build_$compiler
    env CTEST_OUTPUT_ON_FAILURE=1 cmake --build . --target test
}

function make_target() {
    compiler=$1
    target=$2
    . venv/bin/activate
    cd build_$compiler
    cmake --build . --target $target
}

function bb5_pr_cmake_format() {
    make_target intel check-cmake-format
}

function bb5_pr_clang_format() {
    make_target intel check-clang-format
}

function bb5_pr_build_gcc() {
    build_with gcc
}

function bb5_pr_build_intel() {
    build_with intel
}

function bb5_pr_build_nvhpc() {
    build_with nvhpc
}

function bb5_pr_test_gcc() {
    test_with gcc
}

function bb5_pr_test_intel() {
    test_with intel
}

function bb5_pr_test_nvhpc() {
    test_with nvhpc
}

function bb5_pr_build_llvm() {
    build_with llvm
}

function bb5_pr_test_llvm() {
    test_with llvm
}

action=$(echo "$CI_BUILD_NAME" | tr ' ' _)
if [ -z "$action" ] ; then
    for action in "$@" ; do
        "bb5_pr_$action"
    done
else
    "bb5_pr_$action"
fi
