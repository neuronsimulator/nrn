#!/usr/bin/env bash

#if nrniv does not work then see if we can fix the problem
if nrniv -c 'quit()' >& /dev/null ; then
  true
else
  orig_pythonhome="$PYTHONHOME"
  orig_pythonpath="$PYTHONPATH"
  orig_ldlibpath="$LD_LIBRARY_PATH"
  orig_path="$PATH"
  eval "`nrnpyenv.sh`"
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
