@echo off
setlocal
set nrnhome=/cygdrive/%~dp0%/..
set "nrnhome=%nrnhome:\=/%"
set "nrnhome=%nrnhome::=%"
if not "%~1"=="" (
    pushd %~1
    %~dp0/../mingw/usr/bin/sh %~dp0/../lib/mknrndll.sh %nrnhome% "--OUTPUTDIR=%CD%"
    popd
) else (
    %~dp0/../mingw/usr/bin/sh %~dp0/../lib/mknrndll.sh %nrnhome% --OUTPUTDIR=.
)
