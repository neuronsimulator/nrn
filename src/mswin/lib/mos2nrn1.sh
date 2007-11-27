#!/bin/sh

#derived from nrn/bin/mos2nrn2.sh.in
# mos2nrn2 zipfile

NEURONHOME=$N
export NEURONHOME
zipfile=`cygpath -u "$1"`

current="`pwd`"
simdir="$TEMP/$$"

askread="yes"

doclean() {
	if test "$a" = "y" ; then
		echo "removing $simdir"
		cd $simdir/..
		rm -r $simdir 
	else
		echo " not removing $simdir"
	fi      
	sleep 2
	exit 0
}

cleanup() {
	a=y
    if test "$askread" = "yes" ; then
	echo "Clean up by removing directory $simdir ? (n/y):$a"
	read a
    fi
	if test "$a" = "" ; then
		a=y
	fi
	doclean
}

asklaunch() {
	a=C
    if test "$askread" = "yes" ; then
	echo "[C]lean up by removing directory $simdir,
[R]elaunch NEURON,
or immediately e[X]it ? (C/R/X):$a"
	read a
    fi
	case "$a" in
	R|r) a=R;;
	X|x) a=n;;
	*) a=y;;
	esac
}

if  mkdir "$simdir" ; then
	true
else
	echo "Couldn't mkdir $simdir"
	read a
	exit 0
fi
	
# make a file for communication with neuron

cp "$zipfile" "$simdir/nrnzip.zip"
echo "Changing the current directory to $simdir"
cd "$simdir"

unzip -n nrnzip.zip

if [ -r mosinit.hoc ] ; then
	first=./mosinit.hoc
else
	first=`find . -name mosinit.hoc -print |sed -n 1p`
fi

if [ -z "$first" ] ; then
	echo "Missing the mosinit.hoc file"
	cleanup
fi
cd `dirname $first`
first=`basename $first`

if [ -f moslocal.tmp ] ; then
	askread="no"
fi

moddirs="`sed -n '1s;^//moddir;;p' < $first`"
if test "$moddirs" != "" ; then
	modfiles='yes'
	mkdir i686cygwin
	for i in $moddirs ; do
		cp $i/*.[Mm][Oo][Dd] i686cygwin
	done
	cd i686cygwin
else
	modfiles="`ls *.[Mm][Oo][Dd] 2>/dev/null`"
fi

if test "$modfiles" != "" ; then
	sh $N/lib/mknrndl2.sh
	if test -f "nrnmech.dll" ; then
		echo "nrnmech.dll was built successfully."
		if test "$moddirs" != "" ; then
			mv nrnmech.dll ..
			cd ..
		fi
	else
		echo "There was an error in the process of creating nrnmech.dll"
		cleanup
	fi
fi
a=y
if test "$askread" = "yes" ; then
	echo "Run NEURON? (y/n):"
	read a
fi
if [ "$a" != "y" -a "$a" != "" ] ; then
	cleanup
fi
a=R
while test "$a" = "R" ; do
	nrniv mosinit.hoc
	echo ""
	asklaunch
done

doclean

