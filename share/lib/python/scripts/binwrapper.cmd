@echo off
setlocal
set "PY=%~dp0python.exe"
if not exist "%PY%" set "PY=python"
"%PY%" "%~dp0%~n0" %*
exit /b %ERRORLEVEL%
