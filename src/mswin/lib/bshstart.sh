#!/bin/sh
N=`pwd`
if test -f $N/bin/cygpath ; then
  PATH=$N/bin
  cyg=yes
else
  if test -d $N/mingw ; then
    PATH=$N/mingw/bin:$PATH
  fi
  if test -d $N/bin64 ; then
    PATH=$N/bin64:$PATH
  else
    PATH=$N/bin:$PATH
  fi
  cyg=no
fi
export PATH
export N
MPD_CONF_FILE=$N/mpd.conf
export N
export PATH
export MPD_CONF_FILE
#to avoid bash warning, create /tmp if it does not exist
if test $cyg = yes ; then
if test ! -e /tmp ; then
	a=`cygpath --mixed /tmp`
	mkdir -p $a
	if test -d /tmp ; then
		echo "Notice: created $a to avoid bash shell warning in future."
	else
		echo "Notice: could not create $a , the bash shell warning can be ignored."
	fi
fi
fi
cd c:/

if test "$cyg" = "no" ; then 
source set_nrnpyenv.sh
export NEURONHOME=$N
export PYTHONPATH=$PYTHONPATH:$N/lib/python
fi #cyg = no
