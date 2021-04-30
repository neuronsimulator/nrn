#!/bin/bash
set -ex

# standard env variables from ming2 shell
export MSYSTEM_CHOST=x86_64-w64-mingw32
export MSYSTEM=MINGW64
export MINGW_PREFIX=/mingw64
export MINGW_CHOST=x86_64-w64-mingw32
export MSYSTEM_PREFIX=/mingw64
export PATH=/mingw64/bin:$PATH

mkdir -p $BUILD_SOURCESDIRECTORY/build
cd $BUILD_SOURCESDIRECTORY/build

# build and create installer
/mingw64/bin/cmake .. \
	-G 'Unix Makefiles'  \
	-DNRN_ENABLE_MPI_DYNAMIC=ON  \
	-DNRN_ENABLE_MPI=ON  \
	-DCMAKE_PREFIX_PATH='/c/msmpi'  \
	-DNRN_ENABLE_INTERVIEWS=ON  \
	-DNRN_ENABLE_PYTHON=ON  \
	-DNRN_ENABLE_RX3D=ON  \
	-DNRN_RX3D_OPT_LEVEL=2 \
	-DPYTHON_EXECUTABLE=/c/Python35/python.exe \
	-DNRN_ENABLE_PYTHON_DYNAMIC=ON  \
	-DNRN_PYTHON_DYNAMIC='c:/Python35/python.exe;c:/Python36/python.exe;c:/Python37/python.exe;c:/Python38/python.exe;c:/Python39/python.exe;c:/Python27/python.exe'  \
	-DCMAKE_INSTALL_PREFIX='/c/nrn-install' \
	-DMPI_CXX_LIB_NAMES:STRING=msmpi \
	-DMPI_C_LIB_NAMES:STRING=msmpi \
	-DMPI_msmpi_LIBRARY:FILEPATH=c:/msmpi/lib/x64/msmpi.lib
make -j 2
ctest -VV
make install
make setup_exe

# copy installer with fixed name for nightly upload
cp src/mswin/nrn*AMD64.exe $BUILD_SOURCESDIRECTORY/nrn-nightly-AMD64.exe
