@echo off
setlocal
set nrnhome=/cygdrive/%~dp0%/..
set "nrnhome=%nrnhome:\=/%"
set "nrnhome=%nrnhome::=%"
if /I not "%~1" == "-h" (
    if /I not "%~1" == "--help" (
        goto :afterhelp
    )
)

echo Usage: nrnivmodl [options] [mod files or directories with mod files]
echo Options:
echo   -h, --help                       Show this help message and exit.
echo If no MOD files or directories provided then MOD files from current directory are used.
goto :done
:afterhelp
if not "%~1"=="" (
    pushd %~1
    %~dp0/../mingw/usr/bin/sh %~dp0/../lib/mknrndll.sh %nrnhome% "--OUTPUTDIR=%CD%"
    popd
) else (
    %~dp0/../mingw/usr/bin/sh %~dp0/../lib/mknrndll.sh %nrnhome% --OUTPUTDIR=.
)
:done
