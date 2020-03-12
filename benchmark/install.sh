#!/bin/bash

# stop on error
set -e

# build and install directories
export BASE_DIR=$(pwd)/software
export SOURCE_DIR=$BASE_DIR/sources
export INSTALL_DIR=$BASE_DIR/install
mkdir -p $SOURCE_DIR $INSTALL_DIR

# =============================================================================
# 1. Setting source & build dependencies
# =============================================================================

# Clone neuron repository
setup_source() {
    printf "\n----------------- CLONING REPO --------------\n"
    [[ -d $SOURCE_DIR/nrn ]] || git clone --recursive https://github.com/neuronsimulator/nrn.git $SOURCE_DIR/nrn -b benchmark/cineca-v1
    [[ -d $INSTALL_DIR/RINGTEST ]] || git clone https://github.com/nrnhines/ringtest.git $INSTALL_DIR/RINGTEST
    pushd $SOURCE_DIR/nrn && git checkout edad00374773d3128 && popd
}

# Modules to load typically on the cluster environment
# TODO: change the modules based on your environment
setup_modules() {
    printf "\n----------------- SETTING MODULES --------------\n"
    module purge
    module load unstable
    module load cmake/3.15.3 flex/2.6.3 bison/3.0.5 python/3.7.4 gcc/8.3.0 hpe-mpi/2.21
}

# Install python packages if not exist in standard environment
setup_python_packages() {
    printf "\n----------------- SETUP PYTHON PACKAGES --------------\n"
    [[ -d $SOURCE_DIR/venv ]] || python3 -mvenv $SOURCE_DIR/venv
    . $SOURCE_DIR/venv/bin/activate
    pip3 install Jinja2 PyYAML pytest sympy
}


# =============================================================================
# 2. Installing base softwares
# =============================================================================

# Install neuron which is used for building network model. This could be built with GNU
# toolchain as this is only used for input model generation.
install_neuron() {
    mkdir -p $BASE_DIR/build/neuron && pushd $BASE_DIR/build/neuron
    cmake $SOURCE_DIR/nrn \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR/NRN \
        -DNRN_ENABLE_INTERVIEWS=OFF \
        -DNRN_ENABLE_RX3D=OFF \
        -DNRN_ENABLE_CORENEURON=OFF \
        -DNRN_ENABLE_MOD_COMPATIBILITY=ON \
        -DNRN_ENABLE_PYTHON=ON \
        -DPYTHON_EXECUTABLE=$(which python3) \
        -DCMAKE_C_COMPILER=gcc \
        -DCMAKE_CXX_COMPILER=g++ \
        -DCMAKE_BUILD_TYPE=Release
    make -j16 && make install
    popd
}

# Install NMODL which is used for translating DSL to C++ code. This could be built with GNU
# toolchain as this is used as source-to-source compiler.
install_nmodl() {
    mkdir -p $BASE_DIR/build/nmodl && pushd $BASE_DIR/build/nmodl
    cmake $SOURCE_DIR/nrn/external/coreneuron/external/nmodl \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR/NMODL \
        -DPYTHON_EXECUTABLE=$(which python3) \
        -DCMAKE_CXX_COMPILER=g++ \
        -DCMAKE_BUILD_TYPE=Release
    make -j16 && make install
    popd
}

# =============================================================================
# 3. Installing simulation engine
# =============================================================================

# CoreNEURON is used as simulation engine and should be compiled with optimal flags.
# Here we build two configurations with vendor compilers : Intel compiler for CPU
# build and PGI compiler for OpenACC based GPU build. Note that Intel compiler is
# typically used on Intel & AMD platforms to enable auto-vectorisation.

install_coreneuron_cpu()  {
    # TODO change modules
    module load intel/19.0.4
    mkdir -p $BASE_DIR/build/cpu && pushd $BASE_DIR/build/cpu
    cmake $SOURCE_DIR/nrn/external/coreneuron \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR/CPU \
        -DCORENRN_ENABLE_UNIT_TESTS=OFF \
        -DCORENRN_ENABLE_OPENMP=OFF \
        -DCORENRN_ENABLE_NMODL=ON \
        -DCORENRN_NMODL_DIR=$INSTALL_DIR/NMODL \
        -DCMAKE_C_COMPILER=`which icc` \
        -DCMAKE_CXX_COMPILER=`which icpc` \
        -DCMAKE_BUILD_TYPE=Release
    make -j16 && make install
    popd
    # TODO change modules
    module unload intel/19.0.4
}

install_coreneuron_gpu()  {
    # make sure cuda compatible gcc is loaded
    # TODO change modules
    module load pgi/19.4 cuda/10.1.243
    mkdir -p $BASE_DIR/build/gpu && pushd $BASE_DIR/build/gpu
    cmake $SOURCE_DIR/nrn/external/coreneuron \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR/GPU \
        -DCORENRN_ENABLE_UNIT_TESTS=OFF \
        -DCORENRN_ENABLE_OPENMP=OFF \
        -DCORENRN_ENABLE_GPU=ON \
        -DCORENRN_ENABLE_NMODL=ON \
        -DCORENRN_NMODL_DIR=$INSTALL_DIR/NMODL \
        -DCMAKE_C_COMPILER=`which pgcc` \
        -DCMAKE_CXX_COMPILER=`which pgc++` \
        -DCMAKE_BUILD_TYPE=Release
    make -j16 && make install
    popd
    # TODO change modules
    module unload pgi/19.4 cuda/10.1.243
}

setup_source
setup_modules
setup_python_packages
install_neuron
install_nmodl
install_coreneuron_cpu
install_coreneuron_gpu
