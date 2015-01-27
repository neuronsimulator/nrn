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

#if nrniv does not work then see if we can fix the problem
if nrniv -c 'quit()' >& /dev/null ; then
  true
else
  orig_pythonhome="$PYTHONHOME"
  orig_pythonpath="$PYTHONPATH"
  orig_ldlibpath="$LD_LIBRARY_PATH"
  orig_path="$PATH"
  eval "`mk_nrnpyenv.sh`"
  if nrniv -c 'quit()' >& /dev/null ; then
    true
 else
    echo Attempt to fix nrniv failure using: 'eval "`mk_nrnpyenv.sh`"' failed.
    if test "$orig_pythonhome" = "" ; then
      unset PYTHONHOME
    else
      export PYTHONHOME="$orig_pythonhome"
    fi
    if test "$orig_pythonpath" = "" ; then
      unset PYTHONPATH
    else
      export PYTHONPATH="$orig_pythonpath"
    fi
    if test "$orig_ldlibpath" = "" ; then
      unset LD_LIBRARY_PATH
    else
      export LD_LIBRARY_PATH="$orig_ldlibpath"
    fi
    export PATH="$orig_path"
  fi
fi

fi #cyg = no
