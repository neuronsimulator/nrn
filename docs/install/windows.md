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

## Setting up Visual Studio Code

It is highly recommended to use Visual Studio Code for development. You can install it from [https://code.visualstudio.com/](https://code.visualstudio.com/). 

During the development process, you will be using PowerShell, cmd and moreover MSYS2 MINGW64 shell. In order to be able to launch any of these in the IDE, add the following to `.vscode/setting.json` file in the root of the `nrn` repository:

```json
{
    "cmake.configureOnOpen": false,
    "terminal.integrated.profiles.windows": {
    "PowerShell": {
      "source": "PowerShell",
      "icon": "terminal-powershell"
    },
    "Command Prompt": {
      "path": [
        "${env:windir}\\Sysnative\\cmd.exe",
        "${env:windir}\\System32\\cmd.exe"
      ],
      "args": [],
      "icon": "terminal-cmd"
    },
    "MSYS2": {
      "path": "C:\\msys64\\usr\\bin\\bash.exe",
      "args": [
        "--login",
        "-i"
      ],
      "env": {
        "MSYSTEM": "MINGW64",
        "CHERE_INVOKING": "1"
      }
    }
  },
}
```

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

### Windows CI has a new build failure without any code change

GitHub/Azure runners are regularly updated. MSYS2 is already installed on the system with a specfic `pacman` cache at the time the runner images are built. As a consequence, some packages may not play well with the new environment.

First line of attack is to compare successful and failed builds to see what changed. If the issue is related to a new package, one approach is to update the MSYS2 cache in `ci/win_install_deps.cmd` by uncommenting the following line:
```powershell
:: update pacman cache (sometimes required when new GH/Azure runner images are deployed)
:: %MSYS2_ROOT%\usr\bin\pacman -Syy
```

Downsides:
* slower CIs (more time to install new things from cache gradually over time)
* hit issues sooner rather than later (but then we can disable the cache update)


### association.hoc test failed

Unfortunately there is not much we can do about it. This test is very very tricky to handle in the CI and it tests that `.hoc` files can be directly launched with the NEURON installation. We currently have a mitigation in place, we do it in two separate CI steps. Re-running the CI should fix the issue.


