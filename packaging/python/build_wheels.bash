#!/usr/bin/env bash
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

set -xeu

if [ ! -f setup.py ]; then
    echo "Error: setup.py not found. Please launch $0 from the nrn root dir"
    exit 1
fi

# various platform identifiers (as reported by `uname -s`)
PLATFORM_LINUX="Linux"
PLATFORM_MACOS="Darwin"

# path to the (temp) requirements file containing all of the build dependencies
# for NEURON and its submodules
python_requirements_path="$(mktemp -d)/requirements.txt"

clone_nmodl_and_add_requirements() {
    git config --global --add safe.directory /root/nrn
    git submodule update --init --recursive --force --depth 1 -- external/nmodl
    # We only want the _build_ dependencies
    sed -e '/^# runtime dependencies/,$ d' external/nmodl/requirements.txt >> "${python_requirements_path}"
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


configure_linux() {
    echo " - Installing build requirements for Linux"
    pip install 'auditwheel<=6.2.0'
    CMAKE_PREFIX_PATH="/nrnwheel/ncurses;/nrnwheel/readline"
    export CMAKE_PREFIX_PATH
}

configure_macos() {
    echo " - Installing build requirements for MacOS"
    pip install -U 'delocate<=0.13.0,>=0.13.0'

    # We need to "fix" the platform tag if the Python installer is universal2
    # See:
    #     * https://github.com/pypa/setuptools/issues/2520
    #     * https://github.com/neuronsimulator/nrn/pull/1562
    py_platform=$(python -c "import sysconfig; print('%s' % sysconfig.get_platform());")

    echo " - Python platform: ${py_platform}"
    if [[ "${py_platform}" == *"-universal2" ]]; then
        export _PYTHON_HOST_PLATFORM="${py_platform/universal2/$(uname -m)}"
        echo " - Python installation is universal2 and we are on $(uname -m), setting _PYTHON_HOST_PLATFORM to: ${_PYTHON_HOST_PLATFORM}"
        ARCHFLAGS="-arch $(uname -m)"
        export ARCHFLAGS
        echo " - Setting ARCHFLAGS to: ${ARCHFLAGS}"
    fi

    CMAKE_PREFIX_PATH="/opt/nrnwheel/$(uname -m)/ncurses;/opt/nrnwheel/$(uname -m)/readline;/usr/x11"
    export CMAKE_PREFIX_PATH
}

fix_wheel_linux() {
    # For CI runs we skip wheelhouse repairs
    if [ "${SKIP_WHEELHOUSE_REPAIR:-}" = true ] ; then
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
}

fix_wheel_macos() {
    echo " - Calling delocate-listdeps"
    delocate-listdeps dist/*.whl

    echo " - Repairing..."
    # we started clean, there's a single wheel
    delocate-wheel -w wheelhouse -v dist/*.whl
}


build_wheel() {
    platform="$(uname -s)"
    echo "[BUILD WHEEL] Building with interpreter $1 on ${platform}"
    local skip=
    setup_venv "$1"
    (( skip )) && return 0

    if [[ "${platform}" == "${PLATFORM_LINUX}" ]]; then
        configure_linux
    elif [[ "${platform}" == "${PLATFORM_MACOS}" ]]; then
        configure_macos
    fi

    cp packaging/python/build_requirements.txt "${python_requirements_path}"

    CMAKE_DEFS="NRN_MPI_DYNAMIC=$3"
    if [ "$USE_STATIC_READLINE" == "1" ]; then
      CMAKE_DEFS="$CMAKE_DEFS,NRN_BINARY_DIST_BUILD=ON,NRN_WHEEL_STATIC_READLINE=ON"
    fi

    if [ "$2" == "coreneuron" ]; then
        NRN_ENABLE_CORENEURON=ON
        clone_nmodl_and_add_requirements
        CMAKE_DEFS="${CMAKE_DEFS},LINK_AGAINST_PYTHON=OFF"
        if [[ "${platform}" == "${PLATFORM_LINUX}" ]]; then
            CMAKE_DEFS="${CMAKE_DEFS},CORENRN_ENABLE_OPENMP=ON"
        fi
    else
        NRN_ENABLE_CORENEURON=OFF
    fi
    export NRN_ENABLE_CORENEURON

    cat "${python_requirements_path}"
    pip install -r "${python_requirements_path}"
    pip check

    echo " - Building..."
    rm -rf dist build

    # Workaround for https://github.com/pypa/manylinux/issues/1309
    git config --global --add safe.directory "*"

    python setup.py build_ext --cmake-defs="$CMAKE_DEFS" bdist_wheel

    if [[ "${platform}" == "${PLATFORM_LINUX}" ]]; then
        fix_wheel_linux
    elif [[ "${platform}" == "${PLATFORM_MACOS}" ]]; then
        fix_wheel_macos
    fi

    deactivate
}


set_mpi_headers_linux() {
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

    # Check for MPT headers. On Azure, we extract them from a secure file and mount them in the docker image in:
    MPT_INCLUDE_PATH="/nrnwheel/mpt/include"
    if [ -d "$MPT_INCLUDE_PATH" ]; then
        MPI_INCLUDE_HEADERS="${MPI_INCLUDE_HEADERS};${MPT_INCLUDE_PATH}"
    fi

    export MPI_INCLUDE_HEADERS
}

set_mpi_headers_macos() {
    BREW_PREFIX="$(brew --prefix)"
    MPI_POSSIBLE_INCLUDE_HEADERS="${BREW_PREFIX}/opt/openmpi/include ${BREW_PREFIX}/opt/mpich/include"
    MPI_INCLUDE_HEADERS=""
    for dir in $MPI_POSSIBLE_INCLUDE_HEADERS
    do
        if [ -d "${dir}" ]; then
            MPI_INCLUDE_HEADERS="${MPI_INCLUDE_HEADERS};${dir}"
        fi
    done
    export MPI_INCLUDE_HEADERS
}


# platform for which to build the wheel (default: host machine platform)
# supported values: Linux, Darwin, CI
platform=
if [ $# -ge 1 ]; then
    platform=$1
fi

# python version for which wheel to be built; 3* (default) means all python 3 versions
python_wheel_version=
if [ $# -ge 2 ]; then
    python_wheel_version=$2
fi

# enable coreneuron support: "coreneuron" enables support (default: without coreneuron)
# this should be removed/improved once wheel is stable
coreneuron="${3:-}"

# MAIN

case "${platform}" in

  Linux)
    set_mpi_headers_linux
    USE_STATIC_READLINE=1
    python_wheel_version=${python_wheel_version//[-._]/}
    for py_bin in /opt/python/cp${python_wheel_version}*/bin/python; do
        build_wheel "$py_bin" "$coreneuron" "$MPI_INCLUDE_HEADERS"
    done
    ;;

  Darwin)
    set_mpi_headers_macos
    USE_STATIC_READLINE=1
    for py_bin in /Library/Frameworks/Python.framework/Versions/${python_wheel_version}*/bin/python3; do
        build_wheel "$py_bin" "$coreneuron" "$MPI_INCLUDE_HEADERS"
    done
    ;;

  CI)
    if [ "$CI_OS_NAME" == "${PLATFORM_MACOS}" ]; then
        set_mpi_headers_macos
        USE_STATIC_READLINE=0
        build_wheel "$(which python3)" "$coreneuron" "$MPI_INCLUDE_HEADERS"
    elif [ "$CI_OS_NAME" == "${PLATFORM_LINUX}" ]; then
        set_mpi_headers_linux
        USE_STATIC_READLINE=0
        build_wheel "$(which python3)" "$coreneuron" "$MPI_INCLUDE_HEADERS"
    fi
    ls wheelhouse/
    ;;

  *)
    echo "Usage: $(basename "$0") < ${PLATFORM_LINUX} | ${PLATFORM_MACOS} > [python version 36|37|38|39|3*] [coreneuron]"
    exit 1
    ;;

esac
