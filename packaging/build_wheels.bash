#!/bin/bash
set -xe
# A script to loop over the available pythons installed
# on Linux/OSX and build wheels
#
# Note: It should be invoked from nmodl directory
#
# PREREQUESITES:
#  - cmake (>=3.3)
#  - flex
#  - bison
#  - python >= 3.6
#  - C/C++ compiler

if [ ! -f setup.py ]; then
    echo "Error: setup.py not found. Please launch $0 from the nmodl root dir"
    exit 1
fi

setup_venv() {
    local py_bin="$1"
    local py_ver=$("$py_bin" -c "import sys; print('%d%d' % tuple(sys.version_info)[:2])")
    local venv_dir="nmodl_build_venv$py_ver"

    if [ "$py_ver" -lt 36 ]; then
        echo "[SKIP] Python $py_ver not supported"
        skip=1
        return 0
    fi

    echo " - Creating $venv_dir: $py_bin -m venv $venv_dir"
    "$py_bin" -m venv "$venv_dir"
    . "$venv_dir/bin/activate"

    # pep425tags are not available anymore from 0.35
    # temporary workaround until we get smtg stable
    if ! pip install -U pip setuptools "wheel<0.35"; then
        curl https://bootstrap.pypa.io/get-pip.py | python
        pip install -U setuptools "wheel<0.35"
    fi
}


build_wheel_linux() {
    echo "[BUILD WHEEL] Building with interpreter $1"
    local skip=
    setup_venv "$1"
    (( $skip )) && return 0

    echo " - Installing build requirements"
    pip install pip auditwheel setuptools scikit-build
    pip install -r packaging/build_requirements.txt

    echo " - Building..."
    rm -rf dist _skbuild
    # Workaround for https://github.com/pypa/manylinux/issues/1309
    git config --global --add safe.directory "*"
    python setup.py bdist_wheel

    if [ "$TRAVIS" = true ] ; then
        echo " - Skipping repair on Travis..."
        mkdir wheelhouse && cp dist/*.whl wheelhouse/
    else
        echo " - Repairing..."
        auditwheel repair dist/*.whl
    fi

    deactivate
}


build_wheel_osx() {
    echo "[BUILD WHEEL] Building with interpreter $1"
    local skip=
    setup_venv "$1"
    (( $skip )) && return 0

    echo " - Installing build requirements"
    pip install -U delocate scikit-build -r packaging/build_requirements.txt

    echo " - Building..."
    rm -rf dist _skbuild
    python setup.py bdist_wheel

    echo " - Repairing..."
    delocate-wheel -w wheelhouse -v dist/*.whl  # we started clean, there's a single wheel

    deactivate
}

# platform for which wheel to be build
platform=$1

# python version for which wheel to be built; 3* (default) means all python 3 versions
python_wheel_version=3*
if [ ! -z "$2" ]; then
  python_wheel_version=$2
fi

# MAIN

case "$1" in

  linux)
    python_wheel_version=${python_wheel_version//[-._]/}
    for py_bin in /opt/python/cp${python_wheel_version}*/bin/python; do
        build_wheel_linux "$py_bin"
    done
    ;;

  osx)
    for py_bin in /Library/Frameworks/Python.framework/Versions/${python_wheel_version}*/bin/python3; do
        build_wheel_osx "$py_bin"
    done
    ;;

  travis)
    if [ "$TRAVIS_OS_NAME" == "osx" ]; then
        build_wheel_osx $(which python3)
    else
        build_wheel_linux $(which python3)
    fi
    ls wheelhouse/
    ;;

  *)
    echo "Usage: $(basename $0) <linux|osx|travis> [version]"
    exit 1
    ;;

esac
