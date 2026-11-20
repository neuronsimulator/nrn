@echo off
setlocal
rem A venv keeps python.exe next to this script. A prefix install (hosted
rem CPython, C:\Python311) keeps it in the parent directory. pip install
rem --user puts Scripts somewhere else. The script starts with #!python, and
rem pip rewrites that to the installing interpreter. Do not use PATH.
set "PY=%~dp0python.exe"
if not exist "%PY%" set "PY=%~dp0..\python.exe"
if exist "%PY%" goto :run
set "SHEBANG="
set /p SHEBANG=<"%~dp0%~n0"
if not defined SHEBANG goto :nopython
if not "%SHEBANG:~0,2%"=="#!" goto :nopython
set "PY=%SHEBANG:~2%"
if "%PY%"=="" goto :nopython
goto :run
:nopython
echo The NEURON launcher could not find the Python that installed it. 1>&2
exit /b 1
:run
rem Doubled quotes are one quote: nrniv -c "print ""hello""". PowerShell
rem strips those quotes before cmd sees them. docs/install/install_instructions.md
"%PY%" "%~dp0%~n0" %*
exit /b %ERRORLEVEL%
