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

export BUILD_BUILDDIRECTORY="${BUILD_SOURCESDIRECTORY}/build"
export CMAKE_COMMAND=/mingw64/bin/cmake

# build and create installer
${CMAKE_COMMAND} \
    -S "${BUILD_SOURCESDIRECTORY}" \
    -B "${BUILD_BUILDDIRECTORY}" \
    -G Ninja  \
    -DNRN_ENABLE_MPI_DYNAMIC=ON  \
    -DNRN_ENABLE_MPI=ON  \
    -DCMAKE_PREFIX_PATH='/c/msmpi'  \
    -DNRN_ENABLE_INTERVIEWS=ON  \
    -DNRN_ENABLE_PYTHON=ON  \
    -DNRN_ENABLE_RX3D=ON  \
    -DNRN_RX3D_OPT_LEVEL=2 \
    -DNRN_BINARY_DIST_BUILD=ON \
    -DPYTHON_EXECUTABLE=/c/Python39/python.exe \
    -DNRN_ENABLE_PYTHON_DYNAMIC=ON  \
    -DNRN_PYTHON_DYNAMIC='c:/Python39/python.exe;c:/Python310/python.exe;c:/Python311/python.exe;c:/Python312/python.exe;c:/Python313/python.exe'  \
    -DCMAKE_INSTALL_PREFIX='/c/nrn-install' \
    -DMPI_CXX_LIB_NAMES:STRING=msmpi \
    -DMPI_C_LIB_NAMES:STRING=msmpi \
    -DMPI_msmpi_LIBRARY:FILEPATH=c:/msmpi/lib/x64/msmpi.lib
${CMAKE_COMMAND} --build "${BUILD_BUILDDIRECTORY}" --target install
cd "${BUILD_BUILDDIRECTORY}" && ctest -VV || cd -
${CMAKE_COMMAND} --build "${BUILD_BUILDDIRECTORY}" --target setup_exe

# copy installer with fixed name for nightly upload
cp "${BUILD_BUILDDIRECTORY}/src/mswin/"nrn*AMD64.exe "${BUILD_SOURCESDIRECTORY}/nrn-nightly-AMD64.exe"
