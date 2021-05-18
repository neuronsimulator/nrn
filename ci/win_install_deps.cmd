@echo on

:: install all dependencies

:: install python
start /wait msiexec /i python-2.7.msi /norestart /qn TARGETDIR="C:\Python27" ADDLOCAL=ALL || goto :error
python-3.5.exe /passive Include_pip=1 Include_test=0 PrependPath=1 DefaultJustForMeTargetDir=C:\Python35 || goto :error
python-3.6.exe /passive Include_pip=1 Include_test=0 PrependPath=1 DefaultJustForMeTargetDir=C:\Python36 || goto :error
python-3.7.exe /passive Include_pip=1 Include_test=0 PrependPath=1 DefaultJustForMeTargetDir=C:\Python37 || goto :error
python-3.8.exe /passive Include_pip=1 Include_test=0 PrependPath=1 DefaultJustForMeTargetDir=C:\Python38 || goto :error
python-3.9.exe /passive Include_pip=1 Include_test=0 PrependPath=1 DefaultJustForMeTargetDir=C:\Python39 || goto :error

:: fix msvcc version for all python3
pwsh -command "(Get-Content C:\Python35\Lib\distutils\cygwinccompiler.py) -replace 'elif msc_ver == ''1600'':', 'elif msc_ver == ''1900'':' | Out-File C:\Python35\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python36\Lib\distutils\cygwinccompiler.py) -replace 'elif msc_ver == ''1600'':', 'elif msc_ver == ''1916'':' | Out-File C:\Python36\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python37\Lib\distutils\cygwinccompiler.py) -replace 'elif msc_ver == ''1600'':', 'elif msc_ver == ''1900'':' | Out-File C:\Python37\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python38\Lib\distutils\cygwinccompiler.py) -replace 'elif msc_ver == ''1600'':', 'elif msc_ver == ''1916'':' | Out-File C:\Python38\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python39\Lib\distutils\cygwinccompiler.py) -replace 'elif msc_ver == ''1600'':', 'elif msc_ver == ''1927'':' | Out-File C:\Python39\Lib\distutils\cygwinccompiler.py"

:: fix msvc runtime library for all python
pwsh -command "(Get-Content C:\Python35\Lib\distutils\cygwinccompiler.py) -replace 'msvcr100', 'msvcrt' | Out-File C:\Python35\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python36\Lib\distutils\cygwinccompiler.py) -replace 'msvcr100', 'msvcrt' | Out-File C:\Python36\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python37\Lib\distutils\cygwinccompiler.py) -replace 'msvcr100', 'msvcrt' | Out-File C:\Python37\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python38\Lib\distutils\cygwinccompiler.py) -replace 'msvcr100', 'msvcrt' | Out-File C:\Python38\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python39\Lib\distutils\cygwinccompiler.py) -replace 'msvcr100', 'msvcrt' | Out-File C:\Python39\Lib\distutils\cygwinccompiler.py"
pwsh -command "(Get-Content C:\Python27\Lib\distutils\cygwinccompiler.py) -replace 'msvcr90', 'msvcrt' | Out-File C:\Python27\Lib\distutils\cygwinccompiler.py"

:: install numpy
C:\Python35\python.exe -m pip install numpy==1.10.4 || goto :error
C:\Python36\python.exe -m pip install numpy==1.12.1 || goto :error
C:\Python37\python.exe -m pip install numpy==1.14.6 || goto :error
C:\Python38\python.exe -m pip install numpy==1.17.5 || goto :error
C:\Python39\python.exe -m pip install numpy==1.19.3 || goto :error
C:\Python27\python.exe -m pip install numpy || goto :error

:: install pathlib
C:\Python27\python.exe -m pip install pathlib || goto :error

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

:: copy files to necessary directories
cp 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\x64\rc.exe' 'C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\' || goto :error
cp 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\x64\rcdll.dll' 'C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\' || goto :error

:: install msys2 / mingw packages.
:: NOTE: msys2 is already installed in the CI VM image. Otherwise it could be installed with the following line:
:: choco install --no-progress msys2 --params="/InstallDir:%MSYS2_ROOT% /NoUpdate /NoPath" || goto :error
set PATH=%MSYS2_ROOT%\usr\bin;%SystemRoot%\system32;%SystemRoot%;%SystemRoot%\System32\Wbem;%PATH%


%MSYS2_ROOT%\usr\bin\pacman --noconfirm --needed -S --disable-download-timeout ^
git ^
zip ^
unzip ^
base-devel ^
mingw-w64-x86_64-cmake ^
mingw-w64-x86_64-ncurses ^
mingw-w64-x86_64-readline ^
mingw-w64-x86_64-python2 ^
mingw-w64-x86_64-python3 ^
mingw64/mingw-w64-x86_64-cython ^
mingw-w64-x86_64-python3-setuptools ^
mingw-w64-x86_64-python3-pip ^
mingw64/mingw-w64-x86_64-dlfcn ^
mingw-w64-x86_64-toolchain || goto :error

:: if all goes well, go to end
goto :EOF

:: something has failed, teminate with error code
:error
echo ERROR : exiting with error code %errorlevel%
exit /b %errorlevel%
