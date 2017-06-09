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

if test "$PYTHONPATH" != "" ; then #convert dos (';' separated) to posix pathspec
# Only needed when nrniv -c "quit()" succeeds which means
# set_nrnpyenv.sh below does nothing but the PYTHONPATH still has dos syntax.
# Not clear why there must be extra \escapes in second -e expression
export PYTHONPATH=`echo "$PYTHONPATH" | sed -e 's,\([A-Za-z]\):,/\1,g' -e 's,\\\\,/,g' -e 's/;/:/g'`
fi #PYTHONPATH

#source set_nrnpyenv.sh
if python -c 'quit()' >& /dev/null ; then
  eval "`nrnpyenv.sh`"
fi
export NEURONHOME=$N
if test "$PYTHONPATH" = "" ; then
  export PYTHONPATH=$N/lib/python
else
  export PYTHONPATH=$PYTHONPATH:$N/lib/python
fi #PYTHONPATH

fi #cyg = no
