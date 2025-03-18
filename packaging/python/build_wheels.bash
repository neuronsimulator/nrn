#!/usr/bin/env bash
set -eux
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


# setup a venv with a given Python interpreter and activate it
setup_venv() {
    local py_bin="$1"
    py_ver=$("$py_bin" -c "import sys; print('%d%d' % tuple(sys.version_info)[:2])")
    suffix=$("$py_bin" -c "print(str(hash(\"$py_bin\"))[0:8])")
    local venv_dir="nrn_build_venv${py_ver}_${suffix}"

    echo " - Creating $venv_dir: $py_bin -m venv $venv_dir"

    "$py_bin" -m venv "$venv_dir"

    . "$venv_dir/bin/activate"
}


# parse a space-separated list of potential directories
# returns the list of dirs (semicolon-separated) to stdout
parse_dirs() {
    INPUT_VAR="${1}"
    OUTPUT_VAR="${2}"
    for dir in ${INPUT_VAR}
    do
        if [ -d "${dir}" ]; then
            OUTPUT_VAR="${OUTPUT_VAR};${dir}"
        fi
    done
    echo "${OUTPUT_VAR}"
}


# collect various dirs for env vars (Linux)
collect_dirs_linux() {
    # first two are for AlmaLinux 8 (default for manylinux_2_28);
    # second two are for Debian/Ubuntu derivatives
    MPI_POSSIBLE_INCLUDE_HEADERS="/usr/include/openmpi-$(uname -m) /usr/include/mpich-$(uname -m) /usr/lib/$(uname -m)-linux-gnu/openmpi/include /usr/include/$(uname -m)-linux-gnu/mpich"
    NRN_MPI_DYNAMIC="${NRN_MPI_DYNAMIC:-}"
    NRN_MPI_DYNAMIC="$(parse_dirs "${MPI_POSSIBLE_INCLUDE_HEADERS}" "${NRN_MPI_DYNAMIC}")"
    export NRN_MPI_DYNAMIC

    PREFIX_PATH_POSSIBLE_DIRS="/nrnwheel/ncurses /nrnwheel/readline"
    CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}"
    CMAKE_PREFIX_PATH="$(parse_dirs "${PREFIX_PATH_POSSIBLE_DIRS}" "${CMAKE_PREFIX_PATH}")"
    export CMAKE_PREFIX_PATH
}


# collect various dirs for env vars (MacOS)
collect_dirs_macos() {
    BREW_PREFIX="$(brew --prefix)"
    MPI_POSSIBLE_INCLUDE_HEADERS="${BREW_PREFIX}/opt/openmpi/include ${BREW_PREFIX}/opt/mpich/include"
    NRN_MPI_DYNAMIC="${NRN_MPI_DYNAMIC:-}"
    NRN_MPI_DYNAMIC="$(parse_dirs "${MPI_POSSIBLE_INCLUDE_HEADERS}" "${NRN_MPI_DYNAMIC}")"
    export NRN_MPI_DYNAMIC

    PREFIX_PATH_POSSIBLE_DIRS="/opt/nrnwheel/$(uname -m)/ncurses /opt/nrnwheel/$(uname -m)/readline /usr/x11"
    CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}"
    CMAKE_PREFIX_PATH="$(parse_dirs "${PREFIX_PATH_POSSIBLE_DIRS}" "${CMAKE_PREFIX_PATH}")"
    export CMAKE_PREFIX_PATH
}


# for building portable wheels
# if building a Linux portable wheel, docker or podman must be installed
# the preferred container engine can be set using the env variable `CIBW_CONTAINER_ENGINE`
build_wheel_portable() {
    platform="${1}"
    echo "[BUILD WHEEL] Building with cibuildwheel for ${platform}"
    local skip=
    setup_venv "$(command -v python3)"
    (( skip )) && return 0

    python -m pip install cibuildwheel
    echo " - Building..."
    rm -rf _build

    python -m cibuildwheel --debug-traceback --platform "${platform}" --output-dir wheelhouse

    deactivate
}


# for building non-portable wheels
build_wheel_local() {
    interp="$(command -v python3)"
    echo "[BUILD WHEEL] Building with interpreter ${interp}"
    local skip=
    setup_venv "${interp}"
    (( skip )) && return 0

    echo " - Building..."
    rm -rf _build

    if [[ "${2}" == "coreneuron" ]]; then
        NRN_ENABLE_CORENEURON=ON
        # on Linux we also enable OpenMP support
        if [[ "$(uname -s)" == "${PLATFORM_LINUX}" ]]; then
            export CORENRN_ENABLE_OPENMP=ON
        fi
    else
        NRN_ENABLE_CORENEURON=OFF
    fi
    export NRN_ENABLE_CORENEURON

    python -m pip wheel -v --no-deps --config-settings=build-dir=_build --wheel-dir=wheelhouse .

    deactivate
}


# MAIN SEQUENCE

if [ ! -f pyproject.toml ]; then
    echo "Error: pyproject.toml not found. Please launch $0 from the nrn root dir"
    exit 1
fi


# various platform identifiers (as reported by `uname -s`)
PLATFORM_LINUX="Linux"
PLATFORM_MACOS="Darwin"


# platform (operating system) for which to build the wheel
platform="${1}"


# Python version for which to build the wheel; '3*' (default) means all Python 3 versions
python_wheel_version=
if [ $# -ge 2 ]; then
    python_wheel_version="${2}"
    CIBW_BUILD=""
    for ver in ${python_wheel_version}; do
        # we only build cpython-compatible wheels for now
        CIBW_BUILD="${CIBW_BUILD} cp${ver}*"
    done
    export CIBW_BUILD
fi


# enable coreneuron support: "coreneuron" enables support (default: without coreneuron)
# this should be removed/improved once wheel is stable
coreneuron="${3:-}"


case "${platform}" in

  linux | "${PLATFORM_LINUX}")
    export NRN_WHEEL_STATIC_READLINE=ON
    export NRN_BINARY_DIST_BUILD=ON
    build_wheel_portable linux
    ;;

  osx | "${PLATFORM_MACOS}")
    export NRN_WHEEL_STATIC_READLINE=ON
    export NRN_BINARY_DIST_BUILD=ON
    build_wheel_portable macos
    ;;

  CI)
    if [[ "${CI_OS_NAME}" == "osx" || "${CI_OS_NAME}" == "${PLATFORM_MACOS}" ]]; then
        collect_dirs_macos
    else
        collect_dirs_linux
    fi
    build_wheel_local "${coreneuron}"
    ;;

  *)
    echo "Usage: $(basename "$0") < linux | osx | ${PLATFORM_LINUX} | ${PLATFORM_MACOS} > [python version 39|310|3*] [coreneuron]"
    exit 1
    ;;

esac
