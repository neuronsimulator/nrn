#!/bin/sh
# Mac os x intermediate between applescript and neurondemo
echo -e "\033]0;neurondemo\007"

CPU="i686"

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
	echo "No x11 or XDarwin so cannot launch neurondemo."
	echo "Press the "return" key to exit."
	read temp
	exit 0
fi
fi

DYLD_LIBRARY_PATH="${NRNHOME}/${CPU}/lib:${NRNHOME}/../iv/${CPU}/lib:${DYLD_LIBRARY_PATH}"
export DYLD_LIBRARY_PATH

NRNGUI="neurondemo"

if test "x$1" != x ; then
	if test -f "$1" ; then
		dname=`dirname "$1"`
		cd "${dname}"
		arg1=`basename "$1"`
		shift
		${NRNGUI} "$arg1" "$@"
		nex=$?
	else
		cd "$1"
		shift
		${NRNGUI} "$@"
		nex=$?
	fi
else
	cd
	${NRNGUI}
	nex=$?
fi

if test "${nex}" != "0" ; then
	echo "${NRNGUI} exit status was ${nex}"
	echo "Press return key to exit"
	read a
fi

