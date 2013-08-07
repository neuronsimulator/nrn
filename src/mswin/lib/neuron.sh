#!/bin/sh
N="`$1/bin/cygpath -u $1`"
export N
export PYTHONHOME=$N
shift

PATH=$N/bin:/usr/bin:$PATH
export PATH
SH_PATH=$N/bin/sh
export SH_PATH
ARG="`cygpath -u $1`"
DD=`dirname "$ARG"`
FF=`basename "$ARG"`
cd $DD
nrniv $FF
a=$?

if test $a -ne 0 ; then
 echo exit status $a
 echo  Hit return key to exit.
 read a
fi
