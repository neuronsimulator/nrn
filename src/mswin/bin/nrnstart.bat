
set ND=%1
set N=%2
start %ND%\bin\mintty.exe -c %N%/lib/minttyrc %N%/bin/bash --rcfile %N%/lib/nrnstart.bsh %N%/lib/neuron.sh %N% %3 %4 %5 %6 %7

