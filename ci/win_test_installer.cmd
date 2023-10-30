@echo on

:: error variable
set "errorfound="

:: setup environment
set PATH=C:\nrn_test\bin;%PATH%
set PYTHONPATH=C:\nrn_test\lib\python;%PYTHONPATH%
set NEURONHOME=C:\nrn_test

echo %PATH%
echo %PYTHONPATH%
echo %NEURONHOME%

:: Mitigation strategy -> for reasons uknown(thank you Windows), association.hoc.out may not be generated from previous step.
:: If so, try again to generate it. No wait required like previous strategies, we rely on testing entropy from this point on.
if not exist association.hoc.out (start /wait /REALTIME %cd%\ci\association.hoc)

:: test all pythons
C:\Python38\python -c "import neuron; neuron.test(); quit()" || set "errorfound=y"
C:\Python39\python -c "import neuron; neuron.test(); quit()" || set "errorfound=y"
C:\Python310\python -c "import neuron; neuron.test(); quit()" || set "errorfound=y"
C:\Python311\python -c "import neuron; neuron.test(); quit()" || set "errorfound=y"
:: install numpy dependency
python -m pip install numpy
:: run also using whatever is system python
python --version
python -c "import neuron; neuron.test(); quit()" || set "errorfound=y"

:: test python and nrniv
python -c "from neuron import h; s = h.Section(); s.insert('hh'); quit()" || set "errorfound=y"
nrniv -python -c "from neuron import h; s = h.Section(); s.insert('hh'); quit()" || set "errorfound=y"

:: test mpi
mpiexec -n 2 nrniv %cd%\src\parallel\test0.hoc -mpi || set "errorfound=y"
mpiexec -n 2 python %cd%\src\parallel\test0.py -mpi --expected-hosts 2 || set "errorfound=y"

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

:: text rxd, disable until #2585 is fixed
:: python share\lib\python\neuron\rxdtests\run_all.py || set "errorfound=y"

:: Test of association with hoc files. This test is very tricky to handle. We do it in two steps.
:: 2nd step -> check association.hoc output after we've launched 1step in previous CI step
cat association.hoc.out
findstr /i "^hello$" association.hoc.out || set "errorfound=y"

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
echo ERROR : exiting with error code 1 ..
exit 1
