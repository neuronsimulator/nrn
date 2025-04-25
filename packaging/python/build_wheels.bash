#!/usr/bin/env bash
set -xe
# A script to loop over the available pythons installed
# on Linux/OSX and build wheels
#
# Note: It should be invoked from nrn directory
#
# PREREQUESITES:
#  - cmake (>=3.15.0)
#  - flex
#  - bison
#  - python >= 3.8
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

# path to the (temp) requirements file containing all of the build dependencies
# for NEURON and its submodules
python_requirements_path="$(mktemp -d)/requirements.txt"

nmodl_add_requirements() {
    sed -e '/^# runtime dependencies/,$ d' nmodl_requirements.txt >> "${python_requirements_path}"
}


setup_venv() {
    local py_bin="$1"
    py_ver=$("$py_bin" -c "import sys; print('%d%d' % tuple(sys.version_info)[:2])")
    suffix=$("$py_bin" -c "print(str(hash(\"$py_bin\"))[0:8])")
    local venv_dir="nrn_build_venv${py_ver}_${suffix}"

    echo " - Creating $venv_dir: $py_bin -m venv $venv_dir"

    "$py_bin" -m venv "$venv_dir"

    . "$venv_dir/bin/activate"

    if ! pip install -U 'pip<=25.0.1' 'setuptools<=70.3.0' 'wheel<=0.45.1'; then
        curl https://raw.githubusercontent.com/pypa/get-pip/20.3.4/get-pip.py | python
        pip install -U 'setuptools<=70.3.0' 'wheel<=0.45.1'
    fi

}


if [ -z "$ARCHTYPE" ]; then
    ARCHTYPE=$(uname -m)
    export ARCHTYPE
fi

if [[ "$1" == "osx" || ( "$1" == "CI" && "$CI_OS_NAME" == "osx" ) ]]; then
    if [ "$ARCHTYPE" == "universal2" ]; then
        ARCHFLAGS="-arch arm64 -arch x86_64"
    elif [ "$ARCHTYPE" == "arm64" ]; then
        ARCHFLAGS="-arch arm64"
    elif [ "$ARCHTYPE" == "x86_64" ]; then
        ARCHFLAGS="-arch x86_64"
    else
        echo "Error: Cannot determine ARCHFLAGS from ARCHTYPE \"$ARCHTYPE\""
        exit 1
    fi
    export ARCHFLAGS # passed to build_static_readline_osx.bash
fi

if [[ "$1" == "osx" ]]; then
  echo "Building $ARCHTYPE readline and ncurses for macOS"
  bash packaging/python/build_static_readline_osx.bash
  export LDFLAGS="-L/opt/nrnwheel/$ARCHTYPE/readline/lib -L/opt/nrnwheel/$ARCHTYPE/ncurses/lib"
  export CFLAGS="-I/opt/nrnwheel/$ARCHTYPE/readline/include -I/opt/nrnwheel/$ARCHTYPE/ncurses/include"
fi

build_wheel_linux() {
    echo "[BUILD WHEEL] Building with interpreter $1"
    local skip=
    setup_venv "$1"
    (( $skip )) && return 0

    echo " - Installing build requirements"
    pip install 'auditwheel<=6.2.0'
    cp packaging/python/build_requirements.txt "${python_requirements_path}"

    CMAKE_DEFS="NRN_MPI_DYNAMIC=$3"
    if [ "$USE_STATIC_READLINE" == "1" ]; then
      CMAKE_DEFS="$CMAKE_DEFS,NRN_BINARY_DIST_BUILD=ON,NRN_WHEEL_STATIC_READLINE=ON"
    fi

    if [ "$2" == "coreneuron" ]; then
        setup_args="--enable-coreneuron"
        nmodl_add_requirements
        CMAKE_DEFS="${CMAKE_DEFS},NRN_LINK_AGAINST_PYTHON=OFF,CORENRN_ENABLE_OPENMP=ON"
    fi

    cat "${python_requirements_path}"
    pip install -r "${python_requirements_path}"
    pip check

    echo " - Building..."
    rm -rf dist build

    # Workaround for https://github.com/pypa/manylinux/issues/1309
    git config --global --add safe.directory "*"

    python setup.py build_ext --cmake-prefix="/nrnwheel/ncurses;/nrnwheel/readline" --cmake-defs="$CMAKE_DEFS" $setup_args bdist_wheel

    # For CI runs we skip wheelhouse repairs
    if [ "$SKIP_WHEELHOUSE_REPAIR" = true ] ; then
        echo " - Skipping wheelhouse repair ..."
        mkdir wheelhouse && cp dist/*.whl wheelhouse/
    else
        echo " - Auditwheel show"
        auditwheel show dist/*.whl
        echo " - Repairing..."
        # NOTE:
        #   libgomp:  still need work to make sure this robust and usable
        #             currently this will break when coreneuron is used and when
        #             dev environment is not installed. Note that on aarch64 we have
        #             seen issue with libgomp.so and hence we started excluding it.
        #   libnrniv: we ship precompiled version of neurondemo containing libnrnmech.so
        #             which is linked to libnrniv.so. auditwheel manipulate rpaths and
        #             ships an extra copy of libnrniv.so and hence exclude it here.
        auditwheel -v repair dist/*.whl --exclude "libgomp.so.1" --exclude "libnrniv.so"
    fi

    deactivate
}


build_wheel_osx() {
    echo "[BUILD WHEEL] Building with interpreter $1"
    local skip=
    setup_venv "$1"
    (( $skip )) && return 0

    echo " - Installing build requirements"
    cp packaging/python/build_requirements.txt "${python_requirements_path}"

    if [ -z "$MACOSX_DEPLOYMENT_TARGET" ]; then
        MACOSX_DEPLOYMENT_TARGET=$(python -c "import sysconfig; print(sysconfig.get_platform().split('-')[1])")
    fi

    if [ "$ARCHTYPE" == "universal2" ]; then
        UNIVERSAL2=1
    else
        UNIVERSAL2=0
    fi

    CMAKE_DEFS="NRN_MPI_DYNAMIC=$3"
    if [ "$USE_STATIC_READLINE" == "1" ]; then
      CMAKE_DEFS="$CMAKE_DEFS,NRN_BINARY_DIST_BUILD=ON,NRN_WHEEL_STATIC_READLINE=ON"
    fi

    if [ "$2" == "coreneuron" ]; then
        setup_args="--enable-coreneuron"
        nmodl_add_requirements
        CMAKE_DEFS="${CMAKE_DEFS},NRN_LINK_AGAINST_PYTHON=OFF"
    fi

    cat "${python_requirements_path}"
    pip install -U 'delocate<=0.13.0' -r "${python_requirements_path}"
    pip check

    echo " - Building..."
    rm -rf dist build

    if [ "$UNIVERSAL2" == "1" ]; then
        CMAKE_DEFS="${CMAKE_DEFS},CMAKE_OSX_ARCHITECTURES=x86_64;arm64"
    else
        CMAKE_DEFS="${CMAKE_DEFS},CMAKE_OSX_ARCHITECTURES=$ARCHTYPE"
    fi

    PLAT_NAME="macosx_${MACOSX_DEPLOYMENT_TARGET//./_}_$ARCHTYPE"

    python setup.py build_ext --cmake-prefix="/opt/nrnwheel/$ARCHTYPE/readline;/opt/nrnwheel/$ARCHTYPE/ncurses;/usr/x11" --cmake-defs="$CMAKE_DEFS" $setup_args bdist_wheel --plat-name="$PLAT_NAME"

    echo " - Calling delocate-listdeps"
    delocate-listdeps dist/*.whl

    # Remove static .a files from wheel (not needed, avoids delocate issues)
    # ...well... except for libcoreneuron-core.a which is needed by nrnivmodl_core_makefile
    libafiles=$(unzip -t dist/*.whl | sed -n 's,.*\(neuron/\.data/lib/lib.*\.a\)  .*,\1,p' | grep -v 'libcoreneuron-core\.a')
    echo "removing from wheel: $libafiles"
    if test "$libafiles" != "" ; then
        zip -d dist/*.whl $libafiles
    fi

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

# enable coreneuron support: "coreneuron"
# this should be removed/improved once wheel is stable
coreneuron=$3

# MAIN

case "$1" in

  linux)
    MPI_POSSIBLE_INCLUDE_HEADERS="/usr/include/openmpi-$(uname -m) /usr/include/mpich-$(uname -m) /usr/lib/$(uname -m)-linux-gnu/openmpi/include /usr/include/$(uname -m)-linux-gnu/mpich"
    MPI_INCLUDE_HEADERS=""
    for dir in $MPI_POSSIBLE_INCLUDE_HEADERS
    do
        if [ -d "${dir}" ]; then
            MPI_INCLUDE_HEADERS="${MPI_INCLUDE_HEADERS};${dir}"
        fi
    done

    # Check for MPT headers. On Azure, we extract them from a secure file and mount them in the docker image in:
    MPT_INCLUDE_PATH="/nrnwheel/mpt/include"
    if [ -d "$MPT_INCLUDE_PATH" ]; then
        MPI_INCLUDE_HEADERS="${MPI_INCLUDE_HEADERS};${MPT_INCLUDE_PATH}"
    fi
    USE_STATIC_READLINE=1
    python_wheel_version=${python_wheel_version//[-._]/}
    for py_bin in /opt/python/cp${python_wheel_version}*/bin/python; do
        build_wheel_linux "$py_bin" "$coreneuron" "$MPI_INCLUDE_HEADERS"
    done
    ;;

  osx)
    BREW_PREFIX=$(brew --prefix)
    MPI_INCLUDE_HEADERS="${BREW_PREFIX}/opt/openmpi/include;${BREW_PREFIX}/opt/mpich/include"
    USE_STATIC_READLINE=1
    for py_bin in /Library/Frameworks/Python.framework/Versions/${python_wheel_version}*/bin/python3; do
        build_wheel_osx "$py_bin" "$coreneuron" "$MPI_INCLUDE_HEADERS"
    done
    ;;

  CI)
    if [ "$CI_OS_NAME" == "osx" ]; then
        BREW_PREFIX=$(brew --prefix)
        MPI_INCLUDE_HEADERS="${BREW_PREFIX}/opt/openmpi/include;${BREW_PREFIX}/opt/mpich/include"
        MACOSX_DEPLOYMENT_TARGET="10.15"
        export MACOSX_DEPLOYMENT_TARGET
        build_wheel_osx $(which python3) "$coreneuron" "$MPI_INCLUDE_HEADERS"
    else
        # first two are for AlmaLinux 8 (default for manylinux_2_28);
        # second two are for Debian/Ubuntu derivatives
        MPI_POSSIBLE_INCLUDE_HEADERS="/usr/include/openmpi-$(uname -m) /usr/include/mpich-$(uname -m) /usr/lib/$(uname -m)-linux-gnu/openmpi/include /usr/include/$(uname -m)-linux-gnu/mpich"
        MPI_INCLUDE_HEADERS=""
        for dir in $MPI_POSSIBLE_INCLUDE_HEADERS
        do
            if [ -d "${dir}" ]; then
                MPI_INCLUDE_HEADERS="${MPI_INCLUDE_HEADERS};${dir}"
            fi
        done
        build_wheel_linux $(which python3) "$coreneuron" "$MPI_INCLUDE_HEADERS"
    fi
    ls wheelhouse/
    ;;

  *)
    echo "Usage: $(basename $0) < linux | osx > [python version 36|37|38|39|3*]  [coreneuron]"
    exit 1
    ;;

esac
