#!/bin/sh
# this script creates the app bundles so the Finder will allow
# drag and drop, double clicking of apps, and double clicking of
# hoc files.

if test "$1" = "" ; then
	echo "launch_inst needs 3 arguments"
	exit
else
	cpu=$1
	IDIR="$2/.."
	srcdir="$3"
	objdir=`pwd`
fi

bindir="$2/${cpu}/bin"

# Note: .icns files are created by putting something on the screen, e.g
# with neurondemo or with idraw, and using
# /Applications/Utilities/Grab to "Capture/Selection" some area of the
# screen and then copy into the clipboard. Then we paste into
# /Developer/Applications/Utilities/"Icon Composer" and save here. 

# the following creates an app bundle.

mkapp() {
	name=$1
	contents=${IDIR}/${name}.app/Contents
	rm -f -r ${IDIR}/${name}.app
	mkdir ${IDIR}/${name}.app
	mkdir ${contents}
	mkdir ${contents}/Resources
	mkdir ${contents}/MacOS

	cp ${name}.icns ${contents}/Resources
	cp ${name}.info ${contents}/Info.plist
	cp ${objdir}/a.out ${contents}/MacOS
}

cd ${srcdir}
if test "$carbon" = "yes" ; then
	rm -r -f $bindir/nrniv.app
	mkapp nrniv
	mv ${IDIR}/nrniv.app $bindir
	rm $bindir/nrniv.app/Contents/MacOS/a.out
	cp $bindir/nrniv $bindir/nrniv.app/Contents/MacOS
else
	mkapp idraw
fi
mkapp nrngui
mkapp mknrndll
mkapp modlunit
mkapp neurondemo
mkapp mos2nrn
