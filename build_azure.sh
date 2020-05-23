#!/bin/bash
set -ex

export MSYSTEM_CHOST=x86_64-w64-mingw32
export MSYSTEM=MINGW64
export MINGW_PREFIX=/mingw64
export MINGW_CHOST=x86_64-w64-mingw32
export MSYSTEM_PREFIX=/mingw64
export PATH=/mingw64/bin:$PATH

sed -i "s/elif msc_ver == '1600':/elif msc_ver == '1900':/g" /c/python35/lib/distutils/cygwinccompiler.py
sed -i "s/elif msc_ver == '1600':/elif msc_ver == '1916':/g" /c/python36/lib/distutils/cygwinccompiler.py
sed -i "s/elif msc_ver == '1600':/elif msc_ver == '1900':/g" /c/python37/lib/distutils/cygwinccompiler.py
sed -i "s/elif msc_ver == '1600':/elif msc_ver == '1916':/g" /c/python38/lib/distutils/cygwinccompiler.py
sed -i "s/return \['msvcr100'\]/return \['msvcrt'\]/g" /c/python3*/lib/distutils/cygwinccompiler.py

# python 27 cygwinccompiler.py doesn't need to update msc_ver
sed -i "s/return \['msvcr90'\]/return \['msvcrt'\]/g" /c/python2*/lib/distutils/cygwinccompiler.py

cd $BUILD_SOURCESDIRECTORY
mkdir -p build && cd build

/mingw64/bin/cmake .. \
	-G 'Unix Makefiles'  \
	-DNRN_ENABLE_MPI_DYNAMIC=ON  \
	-DNRN_ENABLE_MPI=ON  \
	-DCMAKE_PREFIX_PATH='/c/msmpi'  \
	-DNRN_ENABLE_INTERVIEWS=ON  \
	-DNRN_ENABLE_PYTHON=ON  \
	-DNRN_ENABLE_RX3D=ON  \
	-DPYTHON_EXECUTABLE=/c/python35/python.exe \
	-DNRN_ENABLE_PYTHON_DYNAMIC=ON  \
	-DNRN_PYTHON_DYNAMIC='c:/python35/python.exe;c:/python36/python.exe;c:/python37/python.exe;c:/python38/python.exe;c:/python27/python.exe'  \
	-DCMAKE_INSTALL_PREFIX='/c/nrn-install' \
	-DMPI_CXX_LIB_NAMES:STRING=msmpi \
	-DMPI_C_LIB_NAMES:STRING=msmpi \
	-DMPI_msmpi_LIBRARY:FILEPATH=c:/msmpi/lib/x64/msmpi.lib
make -j
make install
make setup_exe
