# Windows Build

## Pre-requisites

You need to install the following:

*  Chocolatey: [https://chocolatey.org/](https://chocolatey.org/)
*  `pwsh` (grab the latest PowerShell release on GitHub: [https://github.com/PowerShell/PowerShell/releases/latest](https://github.com/PowerShell/PowerShell/releases/latest))
*  Microsoft MPI: https://learn.microsoft.com/en-us/message-passing-interface/microsoft-mpi
*  NSIS (to create the installer): https://nsis.sourceforge.io/Main_Page

## Build Environment Installation

You can follow the outline of the [GitHub CI Workflow](.github/workflow/windows.yml),
replacing some of the steps with manual installations.

### MPI

Follow the above link to download and install MPI.  You should set the following
environment variables for it (please adjust paths if required):
```powershell
$env:MSMPI_BIN="C:\Program Files\Microsoft MPI\Bin\"
$env:MSMPI_INC="C:\Program Files (x86)\Microsoft SDKs\MPI\Include\"
$env:MSMPI_LIB32="C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x86\"
$env:MSMPI_LIB64="C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64\"
```

### Visual Studio and C++ Build Dependencies

These commands should set you up with the basic dependencies for a windows build:
```powershell
choco install -y cmake
choco install -y git
choco install -y sed
choco install -y visualstudio2022community
choco install -y winflexbison3
```

### Python

If you wish to install multiple Python versions, please execute the following commands
repeatedly, and adjust as needed:
```powershell
choco install python311
C:\Python311\python.exe -mpip install -r requirements.txt
```

## How to build NEURON

To build NEURON, please remember to check that the environment variables for MPI are set
correctly, and make note of the Python versions you installed. Configure the build with:
```powershell
cmake -B build -S . `
    -DCMAKE_BUILD_TYPE=Release `
    -DNRN_ENABLE_INTERVIEWS=ON `
    -DNRN_ENABLE_MPI_DYNAMIC=ON `
    -DNRN_ENABLE_MPI=ON `
    -DNRN_ENABLE_PYTHON_DYNAMIC=ON `
    -DNRN_ENABLE_PYTHON=ON `
    -DNRN_ENABLE_RX3D=ON `
    -DNRN_PYTHON_DYNAMIC="c:/Python38/python.exe;c:/Python39/python.exe;c:/Python310/python.exe;c:/Python311/python.exe" `
    -DNRN_RX3D_OPT_LEVEL=2 `
    -DCYTHON_EXECUTABLE="c:/Python38/Scripts/cython.exe"
    -DPYTHON_EXECUTABLE="c:/Python38/python.exe"
```
And then build, using the `Release` configuration:
```
cmake --build build --config Release
```
Note that while there is a `Debug` configuration, it will require debug Python builds.

To create the Windows installer, you need to run:
```bash
make install
make setup_exe
```

Note that by default, the install path is `C:\nrn-install`. When building the installer via `setup_exe`, that is the path that will be used.

## Troubleshooting

### My PR is breaking the GitHub Windows CI

You can use the `live-debug-win` PR tag. To enable it, you have to:
   * add 'live-debug-win' to your PR title
   * push something to your PR branch (note that just re-running the pipeline disregards the title update)

This will setup an ssh session on `tmate.io` and you will get an MSYS2 shell to debug your feature. Be aware that this is limited to the MSYS2 environment. For more adapted debugging, you must set up a local Windows environment.

### The installer is missing some files (includes, libraries, etc.)

We ship out a minimal g++ compiler toolchain to allow nrnivmodl (mknrndll) to build nrnmech.dll.
This is handled by `mingw_files/nrnmingwenv.sh`.

Points of interest:
* `cp_dlls()` -> ship all needed dlls by processing output of `cygcheck`. You can add more files to the caller list if needed.
* `copyinc()` -> ship all needed include files by processing output of `g++ -E`. You can add more files to the caller list if needed.
* `lib` paths for multiple versions of gcc. This is how you would handle it:
  	```bash
	gcclib=mingw64/lib/gcc/x86_64-w64-mingw32/$gccver # gcc 11.2.0 Rev 1
	if test -f /mingw64/lib/libgcc_s.a ; then # gcc 11.2.0 Rev 10
		gcclib=mingw64/lib
	fi
	copy $gcclib '
	libgcc_s.a
	libstdc++.dll.a
	'
	```

### association.hoc test failed

Unfortunately there is not much we can do about it. This test is very very tricky to handle in the CI and it tests that `.hoc` files can be directly launched with the NEURON installation. We currently have a mitigation in place, we do it in two separate CI steps. Re-running the CI should fix the issue.


