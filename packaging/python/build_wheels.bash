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
    local venv_dir="nrn_build_venv${py_ver}"

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
        if [[ -d "${dir}" ]]; then
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


# cibuildwheel does not pass any environmental variables to the container
# unless explicitly specified. Unfortunately, some of them should be set
# dynamically, so we can't put them in pyproject.toml, and hence we set them
# here. All of them can be overridden using its corresponding environmental
# variable. Note that on Linux the variables must also be present in
# pyproject.toml's `[tool.cibuildwheel.linux.environment-pass]` section, or set
# as the `CIBW_ENVIRONMENT_PASS_LINUX` env variable.
set_cibw_environment() {
    platform="${1}"
    if [ "${platform}" = 'macos' ]; then
        declare -A defaults=(
            [CMAKE_PREFIX_PATH]="/opt/nrnwheel/$(uname -m)/ncurses;/opt/nrnwheel/$(uname -m)/readline;/usr/x11"
            [NRN_ENABLE_MPI_DYNAMIC]="ON"
            [NRN_MPI_DYNAMIC]="$(brew --prefix)/opt/openmpi/include;$(brew --prefix)/opt/mpich/include"
            [NRN_WHEEL_STATIC_READLINE]="ON"
            [NRN_ENABLE_CORENEURON]="ON"
            [NRN_BINARY_DIST_BUILD]="ON"
            [NRN_RX3D_OPT_LEVEL]="0"
            # 10.14 is required for full C++17 support according to
            # https://cibuildwheel.readthedocs.io/en/stable/cpp_standards, but it
            # seems that 10.15 is actually needed for std::filesystem::path.
            # 11.0 is required on ARM machines
            [MACOSX_DEPLOYMENT_TARGET]="10.15"
            [CMAKE_C_COMPILER_LAUNCHER]=""
            [CMAKE_CXX_COMPILER_LAUNCHER]=""
            [CCACHE_DIR]=""
        )
    elif [ "${platform}" = 'linux' ]; then
        declare -A defaults=(
            [CMAKE_PREFIX_PATH]="/nrnwheel/ncurses;/nrnwheel/readline"
            [NRN_ENABLE_MPI_DYNAMIC]="ON"
            [NRN_MPI_DYNAMIC]="/usr/include/openmpi-$(uname -m);/usr/include/mpich-$(uname -m)"
            [NRN_WHEEL_STATIC_READLINE]="ON"
            [NRN_ENABLE_CORENEURON]="ON"
            [CORENRN_ENABLE_OPENMP]="ON"
            [NRN_BINARY_DIST_BUILD]="ON"
            [NRN_RX3D_OPT_LEVEL]="0"
            [CMAKE_C_COMPILER_LAUNCHER]=""
            [CMAKE_CXX_COMPILER_LAUNCHER]=""
            [CCACHE_DIR]=""
        )
    fi

    local env_string=""
    for var in "${!defaults[@]}"; do
        local val="${!var:-${defaults[$var]}}"
        env_string+="${var}='${val}' "
    done

    export CIBW_ENVIRONMENT="${env_string}"
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
    rm -rf "${build_dir}"

    if [ "${platform}" = 'linux' ]; then
        NRN_MPI_DYNAMIC="/usr/include/openmpi-$(uname -m);/usr/include/mpich-$(uname -m)"
        # if we are building on Azure, we can use the MPT headers as well
        if [ -n "${TF_BUILD:-}" ]; then
            NRN_MPI_DYNAMIC="${NRN_MPI_DYNAMIC};/host/opt/nrnwheel/mpt/include"
        fi
        export NRN_MPI_DYNAMIC
    fi

    if [ "${platform}" = 'macos' ]; then
        if [ "$(uname -m)" = 'arm64' ]; then
            export MACOSX_DEPLOYMENT_TARGET='11.0'
        fi
    fi

    # use ccache if it is available (it's always available for Linux builds)
    if [ "${platform}" = 'linux' ] || ([ "${platform}" = 'macos' ] && command -v ccache >& /dev/null); then
        export CCACHE_DEBUG=1
        export CMAKE_C_COMPILER_LAUNCHER=ccache
        export CMAKE_CXX_COMPILER_LAUNCHER=ccache
        if [ "${platform}" = 'linux' ]; then
            # the host filesystem is available in the container at `/host`
            export CCACHE_DIR="${CCACHE_DIR:-/host/tmp/ccache}"
            export CCACHE_NODIRECT=
        elif [ "${platform}" = 'macos' ]; then
            export CCACHE_DIR="${CCACHE_DIR:-/tmp/ccache}"
        fi
        CCACHE_STATS_COMMAND="ccache -svv"
        CCACHE_ZERO_COMMAND="ccache -z"
    else
        CCACHE_STATS_COMMAND=""
        CCACHE_ZERO_COMMAND=""
    fi

    set_cibw_environment "${platform}"

    export CIBW_DEBUG_KEEP_CONTAINER=TRUE
    export CIBW_TEST_COMMAND="${CCACHE_STATS_COMMAND} && yes >& /dev/null"
    export CIBW_BEFORE_BUILD="${CCACHE_ZERO_COMMAND} && ${CCACHE_STATS_COMMAND}"
    export CIBW_BUILD_VERBOSITY=1
    python -m cibuildwheel --debug-traceback --platform "${platform}" --output-dir wheelhouse

    deactivate
}


# for building non-portable wheels
build_wheel_local() {
    interp="${1}"
    echo "[BUILD WHEEL] Building with interpreter ${interp}"
    local skip=
    setup_venv "${interp}"
    (( skip )) && return 0

    echo " - Building..."
    rm -rf "${build_dir}"

    # on some distributions, we need a newer pip to be able to use `--config-settings`
    python -m pip install --upgrade pip
    python -m pip wheel -v --no-deps --config-settings=build-dir="${build_dir}" --wheel-dir=wheelhouse .

    deactivate
}


# MAIN SEQUENCE

if [ ! -f pyproject.toml ]; then
    echo "Error: pyproject.toml not found. Please launch $0 from the nrn root dir"
    exit 1
fi


# where the build files will be placed
build_dir="build_wheel"


# various platform identifiers (as reported by `uname -s`)
PLATFORM_LINUX="Linux"
PLATFORM_MACOS="Darwin"


# help message in case of no arguments
help_message="Usage: $(basename "$0") < CI | linux | osx | ${PLATFORM_LINUX} | ${PLATFORM_MACOS} > [python version 39|310|3*|path_to_interp]"

if [[ $# -lt 2 ]]; then
    echo "${help_message}"
    exit 1
fi


# platform (operating system) for which to build the wheel
platform="${1}"


# Python version for which to build the wheel; '3*' means all Python 3 versions
# note that if `platform=CI`, then this represents the _interpreter path_ instead
python_version_or_interpreter="${2}"
if [[ "${platform}" != 'CI' ]]; then
    CIBW_BUILD=""
    for ver in ${python_version_or_interpreter}; do
        # remove any dots since various CI actions require it, and it's easier to do it here
        ver="${ver//./}"
        # we only build cpython-compatible wheels for now
        CIBW_BUILD="${CIBW_BUILD} cp${ver}*"
    done
    export CIBW_BUILD
fi


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
      CI_OS_NAME="${CI_OS_NAME:-$(uname -s)}"
      if [[ "${CI_OS_NAME}" == "osx" || "${CI_OS_NAME}" == "${PLATFORM_MACOS}" ]]; then
        collect_dirs_macos
    else
        collect_dirs_linux
    fi
    build_wheel_local "${python_version_or_interpreter}"
    ;;

  *)
    echo "${help_message}"
    exit 1
    ;;

esac
