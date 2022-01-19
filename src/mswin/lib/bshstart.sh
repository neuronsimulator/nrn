#!/bin/bash
N=`pwd`
if false ; then
  PATH=$N/bin
  cyg=yes
else
  N=`$N/mingw/usr/bin/cygpath -U $N`
  if test -d $N/mingw ; then
    PATH=$N/mingw/usr/bin:$N/mingw/mingw64/bin:$PATH
  fi
  PATH=$N/bin:$PATH
  cyg=no
fi
export PATH
export N

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

if test "$PYTHONPATH" != "" ; then #convert dos (';' separated) to posix pathspec
# Only needed when nrniv -c "quit()" succeeds which means
# set_nrnpyenv.sh below does nothing but the PYTHONPATH still has dos syntax.
export PYTHONPATH=`cygpath -U -p "$PYTHONPATH"`
fi #PYTHONPATH

export NEURONHOME=$N
if test "$PYTHONPATH" = "" ; then
  export PYTHONPATH=$N/lib/python
else
  export PYTHONPATH=$PYTHONPATH:$N/lib/python
fi #PYTHONPATH

eval "`nrnpyenv.sh`"

fi #cyg = no
