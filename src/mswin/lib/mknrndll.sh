#!sh


# get the last argument
for last; do true; done

OUTPUTDIR=.
HASDIR=0
# if the last argument begins with --NEURON_HOME=
if [[ $last == "--OUTPUTDIR="* ]] ; then
OUTPUTDIR=${last#"--OUTPUTDIR="}
HASDIR=1
# remove the last argument
set -- "${@:1:$(($#-1))}"
fi


if test -f $1/bin/cygpath ; then
  N="`$1/bin/cygpath -u $1`"
  PATH=$N/bin
else
  N=$1
  if test -d $N/mingw/mingw64 ; then
    PATH=$N/mingw/mingw64/bin:$PATH
  fi
  if test -d $N/mingw ; then
    PATH=$N/mingw/usr/bin:$PATH
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

rm -f "$OUTPUTDIR/nrnmech.dll"
sh $N/lib/mknrndl2.sh "--OUTPUTDIR=$OUTPUTDIR"

echo ""
if [ -f "$OUTPUTDIR/nrnmech.dll" ] ; then
echo "nrnmech.dll was built successfully."
else
echo "There was an error in the process of creating nrnmech.dll"
fi

if [[ $HASDIR == "0" ]]; then

echo "Press Return key to exit"
read a

fi
