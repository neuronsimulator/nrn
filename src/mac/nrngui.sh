#!/usr/bin/env bash
# Mac os x intermediate between applescript and nrngui
echo -e "\033]0;nrngui\007"

NRNGUI="nrngui"

source set_nrnpyenv.sh

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
	echo "nrngui exit status was $nex"
	echo "Press return key to exit"
	read a
fi
