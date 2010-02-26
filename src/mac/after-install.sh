#!/bin/sh

if test "$1" = "" ; then
	echo "afer-install needs a host_cpu argument"
	exit
fi

if test "$2" = "" ; then
	echo "after-install needs a installation directory argument"
	exit
fi

CPU=$1
prefix=$2
srcdir=$3
ivlibdir=$4
export CPU

NSRC="$3"
export NSRC
NVER="`sh $srcdir/nrnversion.sh`"
NDIR="NEURON-$NVER"
export NDIR
N="$prefix"

# activate macnrn.term and the app bundles
osascript -e 'tell application "Finder"'\
 -e 'activate'\
 -e 'make new Finder window'\
 -e 'set target of Finder window 1 to folder "'$NDIR'" of folder "Applications" of startup disk'\
 -e 'set target of Finder window 1 to folder "bin" of folder "'${CPU}'" of folder "nrn" of folder "'$NDIR'" of folder "Applications" of startup disk'\
 -e 'end tell'

#strip the dylibs
cd ${N}/${CPU}/lib
for i in *.dylib ; do
	if test ! -L $i ; then
		strip -x $i
	fi
done



#remove irrelevant executables
cd ${N}/${CPU}/bin
rm -f hoc_e ivoc mos2nrn nrnoc oc

# and strip the others
for i in memacs modlunit nocmodl nrniv ; do
	strip -x $i
done
if test "$carbon" = "yes" ; then
	strip -x nrniv.app/Contents/MacOS/nrniv
fi

# force rebuild of the neurondemo and clean it up
DEMO="${N}/share/nrn/demo"
rm -f -r ${DEMO}/neuron ${DEMO}/release/${CPU}
./neurondemo << here
quit()
here
if cd ${N}/share/nrn/demo/release/${CPU} ; then
	rm -f *.o *.c *.lo *.mod
	cd .libs
	rm -f *.o
	for i in *.so ; do
		if test ! -L $i ; then
			strip -x $i
		fi
	done
	# make the -dll position independent
	cd ../../..
	sed '/\-dll/s,\-dll "/App.*/share/nrn,-dll "${NRNHOME}/share/nrn,' neuron > ntmp
	mv ntmp neuron
	chmod 755 neuron
fi



# only after an iv installation
if test -d "$ivlibdir" ; then
	cd "${ivlibdir}"
	for i in *.dylib ; do
		if test ! -L $i ; then
			strip -x $i
		fi
	done
	cd ../bin
	rm -f iclass idemo
	if test "$carbon" != "yes" ; then
		strip -x idraw
	fi
fi
