#!/bin/sh
N="`$1/bin/cygpath -u $1`"
export N
shift

PATH=$N/bin:/usr/bin:$PATH
export PATH
SH_PATH=$N/bin/sh
export SH_PATH

#rxvt -fn 16 -fg black -bg white -sl 1000 -e nrniv $* &
nrniv $*
a=$?

if test $a -ne 0 ; then
 echo exit status $a
 echo  Hit return key to exit.
 read a
fi
