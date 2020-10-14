@echo on

:: download all installers

:: python
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile python-3.5.exe https://www.python.org/ftp/python/3.5.4/python-3.5.4-amd64.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile python-3.6.exe https://www.python.org/ftp/python/3.6.8/python-3.6.8-amd64.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile python-3.7.exe https://www.python.org/ftp/python/3.7.7/python-3.7.7-amd64.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile python-3.8.exe https://www.python.org/ftp/python/3.8.2/python-3.8.2-amd64.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile python-2.7.msi https://www.python.org/ftp/python/2.7.17/python-2.7.17.amd64.msi || goto :error

:: mpi
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile msmpisetup.exe https://download.microsoft.com/download/a/5/2/a5207ca5-1203-491a-8fb8-906fd68ae623/msmpisetup.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile msmpisdk.msi https://download.microsoft.com/download/a/5/2/a5207ca5-1203-491a-8fb8-906fd68ae623/msmpisdk.msi || goto :error

:: nsis + plugin
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile nsis-3-05.exe https://downloads.sourceforge.net/project/nsis/NSIS%203/3.05/nsis-3.05-setup.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile EnVar_pugin.zip https://nsis.sourceforge.io/mediawiki/images/7/7f/EnVar_plugin.zip || goto :error

:: if all goes well, go to end
goto :EOF

:: something has failed, teminate with error code
:error
echo ERROR : exiting with error code %errorlevel%
exit /b %errorlevel%
