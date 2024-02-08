#!/usr/bin/env bash
set -ex

# standard env variables from ming2 shell
export MSYSTEM_CHOST=x86_64-w64-mingw32
export MSYSTEM=MINGW64
export MINGW_PREFIX=/mingw64
export MINGW_CHOST=x86_64-w64-mingw32
export MSYSTEM_PREFIX=/mingw64
export PATH=/mingw64/bin:$PATH

# have compatible cython3
python3 -m pip install "cython<3"

# if BUILD_SOURCESDIRECTORY not available, use te root of the repo
if [ -z "$BUILD_SOURCESDIRECTORY" ]; then
	export BUILD_SOURCESDIRECTORY=$(git rev-parse --show-toplevel)
fi
mkdir -p $BUILD_SOURCESDIRECTORY/build
cd $BUILD_SOURCESDIRECTORY/build

# build and create installer
/mingw64/bin/cmake .. \
	-G Ninja  \
	-DNRN_ENABLE_MPI_DYNAMIC=ON  \
	-DNRN_ENABLE_MPI=ON  \
	-DCMAKE_PREFIX_PATH='/c/msmpi'  \
	-DNRN_ENABLE_INTERVIEWS=ON  \
	-DNRN_ENABLE_PYTHON=ON  \
	-DNRN_ENABLE_RX3D=ON  \
	-DNRN_RX3D_OPT_LEVEL=2 \
	-DNRN_BINARY_DIST_BUILD=ON \
	-DPYTHON_EXECUTABLE=/c/Python38/python.exe \
	-DNRN_ENABLE_PYTHON_DYNAMIC=ON  \
	-DNRN_PYTHON_DYNAMIC='c:/Python38/python.exe;c:/Python39/python.exe;c:/Python310/python.exe;c:/Python311/python.exe;c:/Python312/python.exe'  \
	-DCMAKE_INSTALL_PREFIX='/c/nrn-install' \
	-DMPI_CXX_LIB_NAMES:STRING=msmpi \
	-DMPI_C_LIB_NAMES:STRING=msmpi \
	-DMPI_msmpi_LIBRARY:FILEPATH=c:/msmpi/lib/x64/msmpi.lib
ninja install
ctest -VV
ninja setup_exe

# copy installer with fixed name for nightly upload
cp src/mswin/nrn*AMD64.exe $BUILD_SOURCESDIRECTORY/nrn-nightly-AMD64.exe
