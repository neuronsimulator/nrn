@echo on
:: MSVC win_amd64 wheel build dependencies. Not the MinGW setup.exe stack
:: (no MSYS2, NSIS, or extra CPython installers). cibuildwheel / setup-python
:: provide the interpreter.

choco install -y --no-progress ninja winflexbison3 || goto :error

:: Microsoft MPI runtime + SDK at C:\msmpi (same prefix as ci/win_install_deps.cmd).
if not exist msmpisetup.exe (
    pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile msmpisetup.exe https://download.microsoft.com/download/a/5/2/a5207ca5-1203-491a-8fb8-906fd68ae623/msmpisetup.exe || goto :error
)
if not exist msmpisdk.msi (
    pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile msmpisdk.msi https://download.microsoft.com/download/a/5/2/a5207ca5-1203-491a-8fb8-906fd68ae623/msmpisdk.msi || goto :error
)
msmpisetup.exe -unattend -installroot C:\msmpi || goto :error
start /wait msiexec /i msmpisdk.msi /norestart /qn INSTALLDIR="C:\msmpi" ADDLOCAL=ALL || goto :error
if exist C:\msmpi\Bin rename C:\msmpi\Bin bin
if exist C:\msmpi\Lib rename C:\msmpi\Lib lib
if exist C:\msmpi\Include rename C:\msmpi\Include include
if exist c:\Windows\System32\msmpi.dll copy /Y "c:\Windows\System32\msmpi.dll" "c:\msmpi\lib\x64\msmpi.dll" || goto :error

:: Later GHA steps need mpiexec on PATH. The installer writes the machine PATH;
:: GitHub Actions does not reload it unless we append GITHUB_PATH.
if defined GITHUB_PATH (
    if exist C:\msmpi\bin echo C:\msmpi\bin>> "%GITHUB_PATH%"
    if exist C:\msmpi\Bin echo C:\msmpi\Bin>> "%GITHUB_PATH%"
)

goto :EOF

:error
echo ERROR : exiting with error code %errorlevel%
exit /b %errorlevel%
