#!/usr/bin/env bash
# this script creates the app bundles so the Finder will allow
# drag and drop, double clicking of apps, and double clicking of
# hoc files.

if test "$1" = "" ; then
	echo "launch_inst_cmake.sh needs 2 arguments"
	exit
else
	NRN_INSTALL="$1"
	NRN_SRC="$2"
	objdir=`pwd`
fi

# Note: .icns files are created by putting something on the screen, e.g
# with neurondemo or with idraw, and using
# /Applications/Utilities/Grab to "Capture/Selection" some area of the
# screen and then copy into the clipboard. Then we paste into
# /Developer/Applications/Utilities/"Icon Composer" and save here. 

# the following creates an app bundle.
# based on the applescript method of nrn5.4 cvs version 1.3 of 2003-11-25
# returned to this because carbon and i386 finally deprecated out of
# existence as of Mojave 10.14

mkapp() {
	name=$1
	app_path=${NRN_INSTALL}/${name}.app
	rm -f -r ${app_path}
	osacompile -o ${app_path} prototype_applescript_cmake.txt
	cp ${name}.icns ${app_path}/Contents/Resources/droplet.icns
}

cd ${NRN_SRC}/src/mac
mkapp nrngui
mkapp mknrndll
mkapp modlunit
mkapp neurondemo
mkapp mos2nrn
if test -f "${NRN_INSTALL}/bin/idraw" ; then
  # idraw will not exist here if no InterViews or not external submodule
  mkapp idraw
fi
