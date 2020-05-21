#!/bin/bash

set -x

export MSYSTEM_CHOST=x86_64-w64-mingw32
export MSYSTEM=MINGW64
export MINGW_PREFIX=/mingw64
export MINGW_CHOST=x86_64-w64-mingw32
export MSYSTEM_PREFIX=/mingw64
export PATH=/mingw64/bin:$PATH

sed -i "s/elif msc_ver == '1600':/elif msc_ver == '1900':/g" /c/python354/lib/distutils/cygwinccompiler.py
sed -i "s/elif msc_ver == '1600':/elif msc_ver == '1916':/g" /c/python368/lib/distutils/cygwinccompiler.py
#sed -i "s/elif msc_ver == '1600':/elif msc_ver == '1900':/g" /c/python377/lib/distutils/cygwinccompiler.py
#sed -i "s/elif msc_ver == '1600':/elif msc_ver == '1916':/g" /c/python382/lib/distutils/cygwinccompiler.py
sed -i "s/return \['msvcr100'\]/return \['msvcrt'\]/g" /c/python3*/lib/distutils/cygwinccompiler.py

ls

cd $BUILD_SOURCESDIRECTORY
ls
rm -rf build
mkdir -p build
cd build

/mingw64/bin/cmake .. \
	-G 'Unix Makefiles'  \
	-DNRN_ENABLE_MPI_DYNAMIC=ON  \
	-DNRN_ENABLE_MPI=ON  \
	-DCMAKE_PREFIX_PATH='/c/ms-mpi'  \
	-DNRN_ENABLE_INTERVIEWS=ON  \
	-DNRN_ENABLE_PYTHON=ON  \
	-DNRN_ENABLE_RX3D=ON  \
	-DPYTHON_EXECUTABLE=/c/python354/python.exe \
	-DNRN_ENABLE_PYTHON_DYNAMIC=ON  \
	-DNRN_PYTHON_DYNAMIC='c:/python354/python.exe;c:/python368/python.exe'  \
	-DCMAKE_INSTALL_PREFIX='/c/nrn-install' \
	-DMPI_CXX_LIB_NAMES:STRING=msmpi \
	-DMPI_C_LIB_NAMES:STRING=msmpi \
	-DMPI_msmpi_LIBRARY:FILEPATH=c:/ms-mpi/lib/x64/msmpi.lib

make -j VERBOSE=1

echo "-----------------------"
ls share/lib/python/neuron/rxd/geometry3d
ls lib/python/neuron/rxd/geometry3d/
echo "-----------------------"

make install
make setup_exe
