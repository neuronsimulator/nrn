# Windows build

How to compile NEURON in Windows.  
 
## Pre-requisites

You need to follow these steps:

### Step 1: (If installing NEURON on a Windows Virtual Machine) 

Create a new VM 
  
------------------------------------------------------------ 

### Step 2: Install Git 

Git: [https://git-scm.com/](https://git-scm.com/)

Use this: [https://git-scm.com/download/win](https://git-scm.com/download/win)

Note: Do not use: `winget install --id Git.Git -e --source winget` 
 
  
------------------------------------------------------------ 

### Step 3: Install Chocolatey [https://chocolatey.org/](https://chocolatey.org/) by command line: 


Run the following in PowerShell (or pwsh):

```powershell
Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1')) 
```
It will not work in cmd.

Ensure Chocolatey commands are on the path 

Ensuring chocolatey.nupkg is in the lib folder 
 
 
------------------------------------------------------------ 

### Step 4: Install Visual Studio with C++ 

Use this: [https://visualstudio.microsoft.com/vs/features/cplusplus/](https://visualstudio.microsoft.com/vs/features/cplusplus/) 

 
------------------------------------------------------------ 

### Step 5: Get PowerShell 7 or newer version

We will use `pwsh` command - correspond to PowerShell 7. The PowerShell 5 use `powershell` command.  
 
#### Step 5.A: See PowerShell Version Currently Running: 

This must be run inside whichever PowerShell environment you want to test.

`$PSVersionTable` or `get-host|Select-Object`  
 
#### Step 5.B: See available options: 

`winget search Microsoft.PowerShell` 
 
#### Step 5.C: Install New PowerShell 

`winget install --id Microsoft.Powershell --source winget`  
or
`winget install --id Microsoft.Powershell.Preview --source winget` 
 
#### Step 5.D: Ensure you are working with PowerShell 7

Note: Find path file to version 7 (`C:\Program Files\PowerShell\7` or `C:\Program Files\PowerShell\7-preview`) and create a shortcut for this PowerShell 7
Windows will continue to use version 5 if we try to open "PowerShell" 

PS: A shortcut is one option. You could also add it to your path. 

------------------------------------------------------------ 

## Build Environment installation

### Step 6: Git clone
At PowerShell 7, go to 
`C:\Users\User>` and create folder named “Neuron”: 

```
mkdir Neuron
```

Inside the "Neuron” folder, run: 

```
git clone https://github.com/neuronsimulator/nrn
```  

 
Note: `PS C:\Users\User\Neuron> git clone git@github.com:neuronsimulator/nrn nrn` may be an option
 (or it may end up in a no permission issue).  
  
------------------------------------------------------------ 

### Step 7: Downloading Dependencies

As **administrator** from PowerShell 7 (right click the icon and select “Run as Administrator”) at `C:\Users\User\Neuron\nrn\ci` run: 
```
.\win_download_deps.cmd
```
 
Note: You will receive a number of messages in the form below; they do not indicate errors.

```
"C:\Users\User\Neuron\nrn>pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile python-3.8.exe https://www.python.org/ftp/python/3.8.2/python-3.8.2-amd64.exe   || goto :error" 
```
 
------------------------------------------------------------ 

### Step 8: Installing Dependencies

As **administrator** from PowerShell 7, at `C:\Users\User\Neuron\nrn\ci` run: 
```
.\win_install_deps.cmd
``` 

This command will take a while to complete.
  
------------------------------------------------------------ 

### Step 9: (If your Windows doesn't have a Windows Subsystem for Linux)

Install Windows Subsystem for Linux:  wsl.exe --update  

`wsl.exe --list --online` shows you your options and 
`wsl.exe --install Ubuntu-24.04` (or any other) selects an option.
  
------------------------------------------------------------ 

### Step 10: (If you are installing NEURON in a Virtual Machine) 

Turn off your VM and at the host computer (your computer) as **administrator** put in PowerShell 7: 

`
Set-VMProcessor -VMName "NameOfYourVM" -ExposeVirtualizationExtensions $true
`

For example:

```powershell
Set-VMProcessor -VMName "Windows 11 dev environment" -ExposeVirtualizationExtensions $true 
```

 
------------------------------------------------------------ 

 
### Step 11: We need execute the “mingw64.exe” 
Look for "msys64", go to folder “msys64” and execute `mingw64.exe`.

Via folder: open folder `C:\msys64` (probably) and double click the executable `mingw64` 

Via terminal: go to the location of  `mingw64.exe` (`C:\msys64` probably or `\\wsl.localhost\Ubuntu-24.04\mnt\c\msys64`) and execute it 

```
./mingw64.exe
```
 or `/mnt/c/msys64$ ./mingw64.exe `
 
It will open a terminal. It is not PowerShell, it is not CMD, it is a mingw64 teminal.
 
------------------------------------------------------------ 
## How to build NEURON

### Step 12: Creating the exe

Now, inside this terminal (it is a mingw64 terminal, see step above) go to `C:\Users\User\Neuron\nrn\ci` and run 

```
./win_build_cmake.sh 
```
 
It will create an exe file `nrn-nightly-AMD64` (at `C:\Users\User\Neuron\nrn\nrn-nightly-AMD64`).
 
------------------------------------------------------------ 

### Step 13: Running the exe

Double click at `nrn-nightly-AMD64` and it will install NEURON. 
It is a standard installation. 

That means the files created at `C:\Users\User\Neuron\nrn\build\bin` will be coped to:  `C:\nrn\bin` and the "installation" will appear at `C:\nrn` and `C:\nrn-install` 
 
------------------------------------------------------------ 


### Step 14: Using and modifying NEURON file... 
You can replace any new `.dll` file at `C:\nrn\bin` and the NEURON will present the new feature implemented. 
 
Example: 

#### Step 14.A: Checking the `nrngui` 
Run `nrngui` (inside the folder `C:\Users\User\Desktop\NEURON 9.0.0 AMD64` double click on `nrngui`)

It will show something like:

```
NEURON -- VERSION X.X.X 2024-07-09

Duke, Yale, and the BlueBrain Project -- Copyright 1984-2022
See http://neuron.yale.edu/neuron/credits

oc>
```

Close this NEURON window.

#### Step 14.B: Making some changes at `init.cpp`

Open the `init.cpp` file (at `C:\Users\User\Neuron\nrn\src\nrnoc\init.cpp`  or at `C:\msys64\home\User\Neuron\nrn\src\nrnoc\init.cpp`) look for `hoc_last_init(void)` and the follow code inside it:

```C
    if (nrnmpi_myid < 1)
        if (nrn_nobanner_ == 0) {
            Fprintf(stderr, "%s\n", nrn_version(1));
            Fprintf(stderr, "%s\n", banner);
            IGNORE(fflush(stderr));
        }
```


modifying this introduction the print line `printf("\n Now It is my NEURON version!!!! \n\n");` for example: 

```C
     if (nrnmpi_myid < 1)
        if (nrn_nobanner_ == 0) {
            Fprintf(stderr, "%s\n", nrn_version(1));
            printf("\nNow It is my NEURON version!!!! \n\n");
            Fprintf(stderr, "%s\n", banner);
            IGNORE(fflush(stderr));
        }
```
and save.

#### Step 14.C: Compiling the new code.

Compile this new code. We can compile using Step 15. (Do it and come back here) 

#### Step 14.D: Replacing the `libnrniv.dll` file

It will create a new `libnrniv.dll` (and `libnrnmpi_msmpi.dll` and a new `libnrnpython3.XX.dll` and new `nrniv`, but just the first one matters for THIS example) at `nrn\build\bin` (at `C:\Users\User\Neuron\nrn\build\bin` or at `C:\msys64\home\User\Neuron\nrn\build\bin` if you did the installation process here). 

We can replace the old file `libnrniv.dll` at `C:\nrn\bin` for this new one and voila! We get our change implemented!! Let's see

#### Step 14.E: Checking the new `nrngui` 

Run `nrngui` (inside the folder `C:\Users\User\Desktop\NEURON 9.0.0 AMD64` double click in `nrngui`)

```
NEURON -- VERSION X.X.X 2024-07-11

Now it is my NEURON version!!!!

Duke, Yale, and the BlueBrain Project -- Copyright 1984-2022
See http://neuron.yale.edu/neuron/credits

oc>
```

------------------------------------------------------------ 

### Step 15: Compile new NEURON code


PS: Useful commands:  

```powershell
cd /c/Users/User/Neuron/nrn/build
``` 
to reach files outside of `C:\msys64\`.

```
echo $PATH
which gcc 
which cmake 
```
To compile new NEURON code, you can (always from a `mingw64` terminal): 

#### Step 15.A: run win_build_cmake.sh 

From nrn file where the source code is (example: `C:\Users\User\Neuron\nrn\ci` or `C:\msys64\home\User\Neuron\nrn\ci` if you copy a version here) run: 
```
.\win_build_cmake.sh
```
or 
 
#### Step 15.B: Run "cmake .." 

From nrn file where is the source code (example: `C:\Users\User\Neuron\nrn\build` or `C:\msys64\home\User\Neuron\nrn` if you copied a version here - see Step 18) run: 

```
cmake .. -GNinja
```

or 

#### Step 15.C: ninja

Run `ninja` from the build file (example: `C:\Users\User\Neuron\nrn\build` or `C:\msys64\home\User\Neuron\nrn\build` if you copied a version here) 
```
ninja
``` 

### Step 16: Dependencies

Python will not have all dependencies, so you can install them via:  

```
python -m pip install plotly 
python -m pip install matplotlib 
python -m pip install pandas 
``` 


## Creating a new environment for NEURON development 
It may be useful to maintain the original version of NEURON installed (at `C:\home\User\Neuron\nrn` for example) and have a new version that you can use for development. We will set up this new environment and version.

### Step 17: Clone NEURON-git inside `msys64`
We create a new folder inside the `msys64`:  `C:\msys64\home\User\Neuron\nrn`
and clone NEURON git: 
```
git clone https://github.com/neuronsimulator/nrn
```
 
### Step 18: Set the configuration options: 

Get inside `Neuron\nrn\build` file (example: `C:\Users\User\Neuron\nrn\build` or `C:\msys64\home\User\Neuron\nrn\build` if you copy a version here) and do:  
```
cmake .. -GNinja -DCMAKE_INSTALL_PREFIX=install -DNRN_ENABLE_RX3D=ON -DNRN_ENABLE_PYTHON_DYNAMIC=ON -DNRN_ENABLE_MPI_DYNAMIC=ON 
```

Another configuration that can be useful is:
```
cmake .. -GNinja -DCMAKE_INSTALL_PREFIX=install -DNRN_ENABLE_RX3D=ON -DNRN_ENABLE_PYTHON_DYNAMIC=ON -DNRN_ENABLE_MPI_DYNAMIC=ON -DPYTHON_EXECUTABLE=/c/Python312/python.exe -DCMAKE_BUILD_TYPE=FastDebug 
```
 
### Step 19: Compiling  
A `ninja install` and `ninja setup_exe` (instead `make setup_exe`) may be useful/necessary for the first time. 

The command 
`ninja` may be enough for further changes from `C:\msys64\home\User\Neuron\nrn\build` folder. See Step 15.C. 
 
Remembering useful files path: 
```
C:\Users\User\Neuron\nrn\build\src\mswin 
C:\nrn\bin 
C:\Users\User\Desktop\NEURON 9.0.0 AMD64 
C:\Users\User\Neuron\nrn\build\bin 
C:\msys64\home\User\Neuron\nrn\build\bin 
 ```


## Common problems:

Errors in **Step 8**:

The error installing dependencies may be related to hidden errors in step 7, "Downloading Dependencies". Verify that the .exe file related to the error was downloaded correctly. Double-click it and start the installation manually to check it corrupted file is the issue. If the downloaded file is corrupt, download it manually.

----------------------------------------------------- 

Do **Step 10** if you face something like:


```powershell
PS C:\Users\User\Neuron\nrn>  bash ci/win_build_cmake.sh 
: invalid optionke.sh: line 2: set: - 
set: usage: set [-abefhkmnptuvxBCEHPT] [-o option-name] [--] [-] [arg ...] 
ci/win_build_cmake.sh: line 3: $'\r': command not found 
ci/win_build_cmake.sh: line 11: $'\r': command not found 
/usr/bin/python3: No module named pip 
ci/win_build_cmake.sh: line 14: $'\r': command not found 
ci/win_build_cmake.sh: line 46: syntax error: unexpected end of file 
 ```

----------------------------------------------------- 
 
See **Steps 9 and 10** if you face something like:

```powershell
PS C:\Users\User\Neuron\nrn> bash ci/win_build_cmake.sh 
Windows Subsystem for Linux has no installed distributions. 
Distributions can be installed by visiting the Microsoft Store: 
https://aka.ms/wslstore 
PS C:\Users\User\Neuron\nrn> wsl.exe --update 
Installing: Windows Subsystem for Linux 
Windows Subsystem for Linux has been installed. 
PS C:\Users\User\Neuron\nrn> bash ci/win_build_cmake.sh 
Windows Subsystem for Linux has no installed distributions. 
  
Use 'wsl.exe --list --online' to list available distributions 
and 'wsl.exe --install <Distro>' to install. 
  
Distributions can also be installed by visiting the Microsoft Store: 
https://aka.ms/wslstore 
Error code: Bash/Service/CreateInstance/GetDefaultDistro/WSL_E_DEFAULT_DISTRO_NOT_FOUND 
 ```
-------------------------------------------------------- 


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


