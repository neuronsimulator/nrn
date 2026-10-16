:: run installer
start /b /wait .\nrn-nightly-AMD64.exe /S /D=C:\nrn_test

:: take a look
dir C:\nrn_test
tree /F C:\nrn_test\lib\python

:: Test of association with hoc files. This test is very tricky to handle. We do it in two steps
:: 1st step -> launch association.hoc here and test the output in another step
start /B /wait /REALTIME %cd%\ci\association.hoc
