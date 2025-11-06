#!/usr/bin/env bash
set -eux

# standard env variables from ming2 shell
export MSYSTEM_CHOST=x86_64-w64-mingw32
export MSYSTEM=MINGW64
export MINGW_PREFIX=/mingw64
export MINGW_CHOST=x86_64-w64-mingw32
export MSYSTEM_PREFIX=/mingw64
export PATH=/mingw64/bin:$PATH

# have compatible cython3
python3 -m pip install "cython<=3.0.12"

# if BUILD_SOURCESDIRECTORY not available, use the root of the repo
if [ -z "${BUILD_SOURCESDIRECTORY:-}" ]; then
    # this exits with non-zero status code if we're not inside of a git work tree
    if ! git rev-parse --is-inside-work-tree >& /dev/null; then
        printf "Not inside of a git repository, and BUILD_SOURCESDIRECTORY is not set; "
        printf "either change the current directory to be anywhere inside the NEURON directory, "
        echo "or set the BUILD_SOURCESDIRECTORY environment variable to the top-level NEURON source directory"
        exit 1
    fi
    BUILD_SOURCESDIRECTORY="$(git rev-parse --show-toplevel)"
    export BUILD_SOURCESDIRECTORY
fi

export BUILD_BUILDDIRECTORY="${BUILD_SOURCESDIRECTORY}/build/windows"
export CMAKE_COMMAND=/mingw64/bin/cmake

# build and create installer
${CMAKE_COMMAND} --preset windows \
    -S "${BUILD_SOURCESDIRECTORY}" \
    -B "${BUILD_BUILDDIRECTORY}"
${CMAKE_COMMAND} --build --preset windows --target install
ctest --preset windows --output-on-failure --parallel
${CMAKE_COMMAND} --build --preset windows --target setup_exe

# copy installer with fixed name for nightly upload
cp "${BUILD_BUILDDIRECTORY}/src/mswin/"nrn*AMD64.exe "${BUILD_SOURCESDIRECTORY}/nrn-nightly-AMD64.exe"
