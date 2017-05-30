#!/usr/bin/env bash
# Smoke test for NEURON builds travis_build.sh relative/path/to/build if the
# environment variable CONFIG_OPTIONS is set, that is used for the ./configure
# script, otherwise a default is used

set -ex

BUILD_DIR=$1; shift
INSTALL_DIR=$(pwd)/$BUILD_DIR/install

if [[ -z $CONFIG_OPTIONS ]]; then
    CONFIG_OPTIONS="--without-x
             --without-paranrn
             --with-nrnpython=$(which python)
             "
fi

CONFIG_OPTIONS+=" --prefix=$INSTALL_DIR"

configure() {
    local CONFIGURE
    CONFIGURE=$1; shift

    echo "Building with options: $CONFIG_OPTIONS"
    "$CONFIGURE" $CONFIG_OPTIONS
}

git_clean() {
    git clean -ffd
}

build() {
    ./build.sh
    CONFIGURE=$(readlink -f configure)
    mkdir -p "$BUILD_DIR"
    pushd "$BUILD_DIR"
    configure "$CONFIGURE"
    make install
    popd
}

test_install() {
    echo -e 'topology()' | "$INSTALL_DIR/x86_64/bin/neurondemo" | grep -q soma
}

git_clean
build
test_install
