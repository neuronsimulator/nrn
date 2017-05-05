#!/bin/sh

#can save cellbuild session after
host=hines@10.10.0.2
#host=hines@128.178.97.80

# needs to be executed in nrnwinobj

m=/c/marshalnrn64/nrn/bin

mx=$host:mxe/usr/x86_64-w64-mingw32.shared
mxt=$host:mxe/tmp-neuron-x86_64-w64-mingw32.shared

scp $mx/x86_64/bin/libIVhines-3.dll $m
scp $mxt/nrn-7.5/src/modlunit/modlunit.exe $m
scp $mxt/nrn-7.5/src/nmodl/nocmodl.exe $m
scp $mxt/nrn-7.5/src/nrniv/neuron.exe $m
scp $mxt/nrn-7.5/src/mswin/nrniv.dll $m
scp $mxt/nrn-7.5/src/mswin/nrniv.exe $m
scp $mxt/nrn-7.5/src/mswin/libnrnpython1013.dll $m
scp $mxt/nrn-7.5/src/mswin/libnrnmpi.dll $m
scp $mxt/nrn-7.5/src/mswin/hocmodule.dll $m

scp $mx/bin/libgcc_s_seh-1.dll $m
scp $mx/bin/libstdc++-6.dll $m
scp $mx/bin/libwinpthread-1.dll $m
scp $mx/bin/libreadline6.dll $m
scp $mx/bin/libhistory6.dll $m
scp $mx/bin/libtermcap.dll $m
scp $mx/bin/libwinpthread-1.dll $m

for i in $m/*.exe $m/*.dll ; do
  strip $i
done

# but stripping msmpi.dll causes failure of dynamic load of libnrnmpi.dll
cp c:/ms-mpi/lib/x64/msmpi.dll $m

mv $m/hocmodule.dll $m/../lib/python/neuron/hoc.pyd
(cd src/mswin ; /c/Program\ Files\ \(x86\)/NSIS/makensis nrnsetupmingw.nsi)

