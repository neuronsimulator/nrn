# Windows build

## Pre-requisites

You need to install the following:

*  Git: [https://git-scm.com/](https://git-scm.com/)
*  Chocolatey: [https://chocolatey.org/](https://chocolatey.org/)
*  Visual Studio with C++: [https://visualstudio.microsoft.com/vs/features/cplusplus/](https://visualstudio.microsoft.com/vs/features/cplusplus/)
*  `pwsh` (grab the latest PowerShell release on GitHub: [https://github.com/PowerShell/PowerShell/releases/latest](https://github.com/PowerShell/PowerShell/releases/latest))

## Build Environment installation

You can follow the lines of the scripts from the `ci` folder. The scripts can also be launched as-is for local installation. Perform a git clone of the `nrn` repository.

From a powershell with **administrator** priviledges, run:
```powershell
PS C:\workspace\nrn> .\ci\win_download_deps.cmd
```
This will download all required installers and corresponding versions(`Python`, `MPI`, `nsis`)

Then launch the installation script:
```powershell
PS C:\workspace\nrn> .\ci\win_install_deps.cmd
```
in order to:

* install all downloaded Python versions and for each:
  * fix MSVCC version
  * fix MSVC runtime library
  * install numpy 
* install `nsis` and required plugin
* install MPI
* install MSYS2 (via `Chocolatey`) and then MinGW toolchain and required build pacakages

## How to build NEURON

For a complete `build/install/create setup.exe`, in a `MinGW64` shell you can run:
```bash
$ bash ci/win_build_cmake.sh
```
As you can see in the script, a typical configuration would be:
```bash
/mingw64/bin/cmake .. \
	-G 'Unix Makefiles'  \
	-DNRN_ENABLE_MPI_DYNAMIC=ON  \
	-DNRN_ENABLE_MPI=ON  \
	-DCMAKE_PREFIX_PATH='/c/msmpi'  \
	-DNRN_ENABLE_INTERVIEWS=ON  \
	-DNRN_ENABLE_PYTHON=ON  \
	-DNRN_ENABLE_RX3D=ON  \
	-DNRN_RX3D_OPT_LEVEL=2 \
	-DPYTHON_EXECUTABLE=/c/Python38/python.exe \
	-DNRN_ENABLE_PYTHON_DYNAMIC=ON  \
	-DNRN_PYTHON_DYNAMIC='c:/Python38/python.exe;c:/Python39/python.exe;c:/Python310/python.exe;c:/Python311/python.exe'  \
	-DCMAKE_INSTALL_PREFIX='/c/nrn-install' \
	-DMPI_CXX_LIB_NAMES:STRING=msmpi \
	-DMPI_C_LIB_NAMES:STRING=msmpi \
	-DMPI_msmpi_LIBRARY:FILEPATH=c:/msmpi/lib/x64/msmpi.lib
```
To create the Windows installer, you need to run:
```bash
make install
make setup_exe
```

