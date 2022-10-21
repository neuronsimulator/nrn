#!/usr/bin/env bash
if test "$1" = "" ; then
	NDIR=NEURON
else
	NDIR="$1"
fi

cd $HOME
rm -r -f tmpdmg
mkdir tmpdmg
cp -R /Applications/$NDIR tmpdmg
hdiutil create -srcfolder tmpdmg -volname $NDIR -ov $NDIR.dmg
