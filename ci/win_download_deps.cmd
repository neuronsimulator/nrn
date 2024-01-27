@echo on

:: download all installers

:: python
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile python-3.8.exe https://www.python.org/ftp/python/3.8.2/python-3.8.2-amd64.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile python-3.9.exe https://www.python.org/ftp/python/3.9.0/python-3.9.0-amd64.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile python-3.10.exe https://www.python.org/ftp/python/3.10.0/python-3.10.0-amd64.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile python-3.11.exe https://www.python.org/ftp/python/3.11.1/python-3.11.1-amd64.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile python-3.12.exe https://www.python.org/ftp/python/3.12.1/python-3.12.1-amd64.exe || goto :error

:: mpi
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile msmpisetup.exe https://download.microsoft.com/download/a/5/2/a5207ca5-1203-491a-8fb8-906fd68ae623/msmpisetup.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile msmpisdk.msi https://download.microsoft.com/download/a/5/2/a5207ca5-1203-491a-8fb8-906fd68ae623/msmpisdk.msi || goto :error

:: nsis + plugin
curl -LO --retry 4 http://prdownloads.sourceforge.net/nsis/nsis-3.05-setup.exe || goto :error
pwsh -command Invoke-WebRequest -MaximumRetryCount 4 -OutFile EnVar_pugin.zip https://nsis.sourceforge.io/mediawiki/images/7/7f/EnVar_plugin.zip || goto :error

:: if all goes well, go to end
goto :EOF

:: something has failed, teminate with error code
:error
echo ERROR : exiting with error code %errorlevel%
exit /b %errorlevel%
