
if test -f $1/bin/cygpath ; then
  N="`$1/bin/cygpath -u $1`"
  PATH=$N/bin
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
fi
export PATH
export N
NEURONHOME=$N
export NEURONHOME

rm -f nrnmech.dll
sh $N/lib/mknrndl2.sh

echo ""
if [ -f nrnmech.dll ] ; then
echo "nrnmech.dll was built successfully."
else
echo "There was an error in the process of creating nrnmech.dll"
fi
echo "Press Return key to exit"
read a

