#!/usr/bin/env bash
# Mac os x intermediate between applescript and nrnivmodl
echo -e "\033]0;mknrndll\007"

NRNIVMODL="nrnivmodl"

if test "x$1" != x ; then
	if test -f "$1" ; then
		dname=`dirname "$1"`
		cd "${dname}"
		files=""
		while test $# -gt 0 ; do
			files="$files "`basename "$1"`
			shift
		done
		${NRNIVMODL} $files
	else
		cd "$1"
		shift
		${NRNIVMODL} "$@"
	fi
else
	echo "drag a folder or set of mod files onto the mknrndll script"
fi

echo "Press 'return' key to close"
read a
