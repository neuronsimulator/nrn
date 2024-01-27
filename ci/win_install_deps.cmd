@echo on

:: install all dependencies

:: install python
python-3.8.exe /passive Include_pip=1 Include_test=0 PrependPath=1 DefaultJustForMeTargetDir=C:\Python38 || goto :error
python-3.9.exe /passive Include_pip=1 Include_test=0 PrependPath=1 DefaultJustForMeTargetDir=C:\Python39 || goto :error
python-3.10.exe /passive Include_pip=1 Include_test=0 PrependPath=1 DefaultJustForMeTargetDir=C:\Python310 || goto :error
python-3.11.exe /passive Include_pip=1 Include_test=0 PrependPath=1 DefaultJustForMeTargetDir=C:\Python311 || goto :error
python-3.12.exe /passive Include_pip=1 Include_test=0 PrependPath=1 DefaultJustForMeTargetDir=C:\Python312 || goto :error

:: fix msvcc version for all python3
pwsh -command "(Get-Content C:\Python38\Lib\distutils\cygwinccompiler.py) -replace 'elif msc_ver == ''1600'':', 'elif msc_ver == ''1916'':' | Out-File C:\Python38\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python39\Lib\distutils\cygwinccompiler.py) -replace 'elif msc_ver == ''1600'':', 'elif msc_ver == ''1927'':' | Out-File C:\Python39\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python310\Lib\distutils\cygwinccompiler.py) -replace 'elif msc_ver == ''1600'':', 'elif msc_ver == ''1929'':' | Out-File C:\Python310\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python311\Lib\distutils\cygwinccompiler.py) -replace 'elif msc_ver == ''1600'':', 'elif msc_ver == ''1934'':' | Out-File C:\Python311\Lib\distutils\cygwinccompiler.py"


:: fix msvc runtime library for all python
pwsh -command "(Get-Content C:\Python38\Lib\distutils\cygwinccompiler.py) -replace 'msvcr100', 'msvcrt' | Out-File C:\Python38\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python39\Lib\distutils\cygwinccompiler.py) -replace 'msvcr100', 'msvcrt' | Out-File C:\Python39\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python310\Lib\distutils\cygwinccompiler.py) -replace 'msvcr100', 'msvcrt' | Out-File C:\Python310\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python311\Lib\distutils\cygwinccompiler.py) -replace 'msvcr100', 'msvcrt' | Out-File C:\Python311\Lib\distutils\cygwinccompiler.py"

:: install numpy
C:\Python38\python.exe -m pip install  numpy==1.17.5 "cython < 3" || goto :error
C:\Python39\python.exe -m pip install  numpy==1.19.3 "cython < 3" || goto :error
C:\Python310\python.exe -m pip install numpy==1.21.3 "cython < 3" || goto :error
C:\Python311\python.exe -m pip install numpy==1.23.5 "cython < 3" || goto :error
C:\Python312\python.exe -m pip install numpy==1.26.3 "cython < 3" || goto :error
C:\Python312\python.exe -m pip install setuptools || goto :error

:: install nsis
nsis-3.05-setup.exe /S || goto :error
pwsh -command Expand-Archive EnVar_pugin.zip -DestinationPath "${env:ProgramFiles(x86)}\NSIS" || goto :error

:: install mpi
msmpisetup.exe -unattend -installroot C:\msmpi || goto :error
start /wait msiexec /i msmpisdk.msi /norestart /qn INSTALLDIR="C:\msmpi" ADDLOCAL=ALL || goto :error
rename C:\msmpi\Bin  bin || goto :error
rename C:\msmpi\Lib  lib || goto :error
rename C:\msmpi\Include include || goto :error
copy "c:\Windows\System32\msmpi.dll" "c:\msmpi\lib\x64\msmpi.dll" || goto :error
copy "c:\Windows\SysWoW64\msmpi.dll" "c:\msmpi\lib\x86\msmpi.dll" || goto :error

:: check if MSYS2_ROOT is set, otherwise set it to C:\msys64 (default)
if "%MSYS2_ROOT%"=="" set MSYS2_ROOT=C:\msys64

:: install msys2 / mingw packages.
:: NOTE: msys2 is already installed in the CI VM image. We check if msys2 is not installed, then download and install it with choco
if not exist "%MSYS2_ROOT%\usr\bin\bash.exe" (
    choco install -y --no-progress msys2 --params="/InstallDir:%MSYS2_ROOT% /NoUpdate /NoPath" || goto :error
)
set PATH=%MSYS2_ROOT%\usr\bin;%SystemRoot%\system32;%SystemRoot%;%SystemRoot%\System32\Wbem;%PATH%

:: update pacman cache (sometimes required when new GH/Azure runner images are deployed)
:: %MSYS2_ROOT%\usr\bin\pacman -Syy

%MSYS2_ROOT%\usr\bin\pacman --noconfirm --needed -S --disable-download-timeout ^
cmake ^
git ^
zip ^
unzip ^
base-devel ^
mingw-w64-x86_64-cmake ^
mingw-w64-x86_64-ninja ^
mingw-w64-x86_64-ncurses ^
mingw-w64-x86_64-readline ^
mingw-w64-x86_64-python3 ^
mingw-w64-x86_64-python3-setuptools ^
mingw-w64-x86_64-python3-packaging ^
mingw-w64-x86_64-python3-pip ^
mingw64/mingw-w64-x86_64-dlfcn ^
mingw-w64-x86_64-toolchain || goto :error

:: if all goes well, go to end
goto :EOF

:: something has failed, teminate with error code
:error
echo ERROR : exiting with error code %errorlevel%
exit /b %errorlevel%
