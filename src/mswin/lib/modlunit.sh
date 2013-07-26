if test -f $1/bin/cygpath ; then
  N="`$1/bin/cygpath -u $1`"
  PATH=$N/bin
  fname="`cygpath -u $2|sed 's/\.[mM][oO][dD]$/.mod/'`"
  cyg=yes
else
  N=$1
  if test -d $N/mingw ; then
    PATH=$N/mingw/bin:$PATH
  fi
  if test -d $N/bin64 ; then
    PATH=$N/bin64:$PATH
  else
    PATH=$N/bin:$PATH
  fi
  fname="`echo $2|sed 's/\.[mM][oO][dD]$/.mod/'`"
  cyg=no
fi
export PATH
export N
NEURONHOME=$N
export NEURONHOME

if test $cyg = yes ; then
if modlunit $fname ; then
echo ""
else
echo ""
echo "Press Return key to exit"
read a
fi
#read a
else
  modlunit $fname
fi
