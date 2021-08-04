#!/bin/bash
set -xe
# A script to loop over the available pythons installed
# on Linux/OSX and build wheels
#
# Note: It should be invoked from nrn directory
#
# PREREQUESITES:
#  - cmake (>=3.5)
#  - flex
#  - bison
#  - python >= 3.6
#  - cython
#  - MPI
#  - X11
#  - C/C++ compiler
#  - ncurses

set -e

if [ ! -f setup.py ]; then
    echo "Error: setup.py not found. Please launch $0 from the nrn root dir"
    exit 1
fi

py_ver=""

setup_venv() {
    local py_bin="$1"
    py_ver=$("$py_bin" -c "import sys; print('%d%d' % tuple(sys.version_info)[:2])")
    suffix=$("$py_bin" -c "print(str(hash(\"$py_bin\"))[0:8])")
    local venv_dir="nrn_build_venv${py_ver}_${suffix}"

    echo " - Creating $venv_dir: $py_bin -m venv $venv_dir"

    "$py_bin" -m venv "$venv_dir"

    . "$venv_dir/bin/activate"

    if ! pip install -U pip setuptools wheel; then
        curl https://raw.githubusercontent.com/pypa/get-pip/20.3.4/get-pip.py | python
        pip install -U setuptools wheel
    fi

}


pip_numpy_install() {
    # numpy is special as we want the minimum wheel version
    numpy_ver="numpy"
    case "$py_ver" in
      36) numpy_ver="numpy==1.12.1" ;;
      37) numpy_ver="numpy==1.14.6" ;;
      38) numpy_ver="numpy==1.17.5" ;;
      39) numpy_ver="numpy==1.19.3" ;;
      *) numpy_ver="numpy";;
    esac
    echo " - pip install $numpy_ver"
    pip install $numpy_ver
}

build_wheel_linux() {
    echo "[BUILD WHEEL] Building with interpreter $1"
    local skip=
    setup_venv "$1"
    (( $skip )) && return 0

    echo " - Installing build requirements"
    #auditwheel needs to be installed with python3
    pip install auditwheel
    pip install -r packaging/python/build_requirements.txt
    pip_numpy_install

    echo " - Building..."
    rm -rf dist build
    if [ "$2" == "--bare" ]; then
        python setup.py bdist_wheel
    else
        CMAKE_DEFS="NRN_MPI_DYNAMIC=$3"
        if [ "$USE_STATIC_READLINE" == "1" ]; then
          CMAKE_DEFS="$CMAKE_DEFS,NRN_WHEEL_STATIC_READLINE=ON"
        fi
        python setup.py build_ext --cmake-prefix="/nrnwheel/ncurses;/nrnwheel/readline" --cmake-defs="$CMAKE_DEFS" bdist_wheel
    fi

    # For CI runs we skip wheelhouse repairs
    if [ "$SKIP_WHEELHOUSE_REPAIR" = true ] ; then
        echo " - Skipping wheelhouse repair ..."
        mkdir wheelhouse && cp dist/*.whl wheelhouse/
    else
        echo " - Auditwheel show"
        auditwheel show dist/*.whl
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
    pip install -U delocate -r packaging/python/build_requirements.txt
    pip_numpy_install

    echo " - Building..."
    rm -rf dist build
    if [ "$2" == "--bare" ]; then
        python setup.py bdist_wheel
    else
        CMAKE_DEFS="NRN_MPI_DYNAMIC=$3"
        if [ "$USE_STATIC_READLINE" == "1" ]; then
          CMAKE_DEFS="$CMAKE_DEFS,NRN_WHEEL_STATIC_READLINE=ON"
        fi
        python setup.py build_ext --cmake-prefix="/opt/nrnwheel/ncurses;/opt/nrnwheel/readline" --cmake-defs="$CMAKE_DEFS" bdist_wheel
    fi

    echo " - Calling delocate-listdeps"
    delocate-listdeps dist/*.whl

    echo " - Repairing..."
    delocate-wheel -w wheelhouse -v dist/*.whl  # we started clean, there's a single wheel

    deactivate
}

# platform for which wheel to be build
platform=$1

# python version for which wheel to be built; 3* (default) means all python 3 versions
python_wheel_version=
if [ ! -z "$2" ]; then
  python_wheel_version=$2
fi

# if to build non-dynamic mpi wheel (TODO: should be removed)
bare=$3

# MAIN

case "$1" in

  linux)
    # include here /nrnwheel/mpt/include if have MPT headers
    MPI_INCLUDE_HEADERS="/nrnwheel/openmpi/include;/nrnwheel/mpich/include"
    USE_STATIC_READLINE=1
    python_wheel_version=${python_wheel_version//[-._]/}
    for py_bin in /opt/python/cp${python_wheel_version}*/bin/python; do
        build_wheel_linux "$py_bin" "$bare" "$MPI_INCLUDE_HEADERS"
    done
    ;;

  osx)
    MPI_INCLUDE_HEADERS="/usr/local/opt/openmpi/include;/usr/local/opt/mpich/include"
    USE_STATIC_READLINE=1
    for py_bin in /Library/Frameworks/Python.framework/Versions/${python_wheel_version}*/bin/python3; do
        build_wheel_osx "$py_bin" "$bare" "$MPI_INCLUDE_HEADERS"
    done
    ;;

  CI)
    if [ "$CI_OS_NAME" == "osx" ]; then
        MPI_INCLUDE_HEADERS="/usr/local/opt/openmpi/include;/usr/local/opt/mpich/include"
        build_wheel_osx $(which python3) "$bare" "$MPI_INCLUDE_HEADERS"
    else
        MPI_INCLUDE_HEADERS="/usr/lib/x86_64-linux-gnu/openmpi/include;/usr/include/mpich"
        build_wheel_linux $(which python3) "$bare" "$MPI_INCLUDE_HEADERS"
    fi
    ls wheelhouse/
    ;;

  *)
    echo "Usage: $(basename $0) < linux | osx > [--bare]"
    exit 1
    ;;

esac
