#!/bin/sh
# Mac os x intermediate between applescript and mos2nrn
echo -e "\033]0;NEURON launched from browser\007"

CPU="i686"

if test "$1" = "" ; then
	echo "The purpose of mos2nrn is to unzip, build, and launch
	models downloaded from the ModelDB model database at
	http://senselab.med.yale.edu."
	echo "Drop a nrnzip (or zip) file containing a NEURON model onto mos2nrn."
	echo "Press the return key to exit."
	read a
	exit 0
fi

if test "$nrncarbon" != "yes" ; then
if test -d /Applications/Utilities/x11.app ; then
	/usr/bin/open -a /Applications/Utilities/x11.app
	DISPLAY=":0.0"
	export DISPLAY
elif test -d /Applications/x11.app ; then
	/usr/bin/open -a /Applications/x11.app
	DISPLAY=":0.0"
	export DISPLAY
elif test -d /Applications/OroborOS* ; then
	temp="`ps -ax | grep Orobor | grep -v grep`"
	running=no
	case $temp in
	*Orobor*) running=yes;;
	esac
	if test -d /Applications/OroborOS*.app ; then
		/usr/bin/open -a /Applications/Orobor*.app
	else
		/usr/bin/open -a /Applications/Orobor*/Orobor*.app
	fi
	DISPLAY=":0.0"
	export DISPLAY
	if test "$running" = "no" ; then
		echo "After Oroboros starts up, press the "return" key."
		read temp
	fi 
elif test -d /Applications/XDarwin.app ; then
	temp="`ps -ax | grep XDar`"
	running=no
	case $temp in
	*XDarwin*) running=yes;;
	esac 
	/usr/bin/open -a /Applications/XDarwin.app
	DISPLAY=":0.0"
	export DISPLAY
	if test "$running" = "no" ; then
		echo "After XDarwin starts up, press the "return" key."
		read temp
	fi
else
	echo "No x11 or XDarwin so cannot launch mos2nrn."
	echo "Press the "return" key to exit."
	read temp
	exit 0
fi
fi

MOS2NRN="mos2nrn2.sh"

DYLD_LIBRARY_PATH="${NRNHOME}/${CPU}/lib:${NRNHOME}/../iv/${CPU}/lib:${DYLD_LIBRARY_PATH}"
export DYLD_LIBRARY_PATH

"${MOS2NRN}" "$1" "/tmp/mos2nrn".$$ "0"
