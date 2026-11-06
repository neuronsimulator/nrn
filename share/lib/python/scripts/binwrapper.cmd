@echo off
setlocal
rem A venv keeps python.exe next to this script. A prefix install (hosted
rem CPython, C:\Python311) keeps it in the parent directory. PATH is last:
rem setup-python leaves `python` as 3.14, which is not the wheel's interpreter.
set "PY=%~dp0python.exe"
if not exist "%PY%" set "PY=%~dp0..\python.exe"
if not exist "%PY%" set "PY=python"
"%PY%" "%~dp0%~n0" %*
exit /b %ERRORLEVEL%
