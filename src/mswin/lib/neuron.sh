#!/bin/sh
N="`$1/bin/cygpath -u $1`"
export N
shift

PATH=$N/bin:/usr/bin:$PATH
export PATH
SH_PATH=$N/bin/sh
export SH_PATH

rxvt -fn 16 -fg black -bg white -sl 1000 -e nrniv $*
#the following indirection is to avoid the terminal from closing
#when neuron exits immediately because of a bad dll so you get a chance
#to see the error message.

#rxvt -fn 16 -fg black -bg white -sl 1000 -e sh $N/lib/neuron2.sh $*
