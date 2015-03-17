#!/bin/bash
export TEMP="$1"
shift
export N="$1"
shift

export NEURONHOME="$N"
export PATH="$N/bin:$N/mingw/bin:$PATH"
echo $N
echo $PATH
source set_nrnpyenv.sh
export PYTHONPATH="$PYTHONPATH:$N/lib/python"
export MINGW='yes'

$N/lib/mos2nrn1.sh "$1"
a=$?
if test $a -ne 0 ; then
  echo'
mos2nrn.exe exited abnormally. Press the return key to close this window.
'
  read a
fi 
