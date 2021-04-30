@echo on

:: error variable
set "errorfound="

:: install installer
:: TODO : need to fix this as next command will not wait till installer finishes
start /b /wait .\nrn-nightly-AMD64.exe /S /D=C:\nrn_test

:: take a look
dir C:\nrn_test
tree /F C:\nrn_test\lib\python

:: setup environment
set PATH=C:\nrn_test\bin;%PATH%
set PYTHONPATH=C:\nrn_test\lib\python;%PYTHONPATH%
set NEURONHOME=C:\nrn_test

echo %PATH%
echo %PYTHONPATH%
echo %NEURONHOME%

:: test all pythons
C:\Python27\python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()" || set "errorfound=y"
C:\Python35\python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()" || set "errorfound=y"
C:\Python36\python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()" || set "errorfound=y"
C:\Python37\python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()" || set "errorfound=y"
C:\Python38\python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()" || set "errorfound=y"
C:\Python39\python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()" || set "errorfound=y"

:: install numpy dependency
python -m pip install numpy
:: run also using whatever is system python
python --version
python -c "import neuron; neuron.test(); neuron.test_rxd(); quit()" || set "errorfound=y"

:: test python and nrniv
python -c "from neuron import h; s = h.Section(); s.insert('hh'); quit()" || set "errorfound=y"
nrniv -python -c "from neuron import h; s = h.Section(); s.insert('hh'); quit()" || set "errorfound=y"

:: test mpi
mpiexec -n 2 nrniv %cd%\src\parallel\test0.hoc -mpi || set "errorfound=y"
mpiexec -n 2 python %cd%\src\parallel\test0.py -mpi --expected-hosts 2 || set "errorfound=y"

:: test of association with hoc files
del temp.txt
echo wopen("temp.txt") > .\temp.hoc
echo fprint("hello\n") >> .\temp.hoc
echo wopen() >> .\temp.hoc
echo quit() >> .\temp.hoc
start .\temp.hoc
ping -n 10 127.0.0.1
cat temp.txt
findstr /i "^hello$" temp.txt || set "errorfound=y"

:: setup for mknrndll/nrnivmodl
set N=C:\nrn_test
set PATH=C:\nrn_test\mingw\usr\bin;%PATH%

:: test mknrndll
copy /A share\examples\nrniv\nmodl\cacum.mod .
C:\nrn_test\mingw\usr\bin\bash -c "mknrndll" || set "errorfound=y"
python -c "import neuron; from neuron import h; s = h.Section(); s.insert('cacum'); print('cacum inserted'); quit()" || set "errorfound=y"

:: test nrnivmodl
rm -f cacum* mod_func* nrnmech.dll
copy /A share\examples\nrniv\nmodl\cacum.mod .
call nrnivmodl
echo "nrnivmodl successfull"
python -c "import neuron; from neuron import h; s = h.Section(); s.insert('cacum'); print('cacum inserted'); quit()" || set "errorfound=y"

echo "All tests finished!"

:: uninstall neuron
C:\nrn_test\uninstall /S || set "errorfound=y"
echo "Uninstalled NEURON"

:: if something failed, exit with error
if defined errorfound (goto :error)

:: if all goes well, go to end
goto :EOF

:: something has failed, teminate with error code
:error
echo ERROR : exiting with error code %errorlevel% ..
exit /b %errorlevel%
