#!/usr/bin/env bash
# Mac os x intermediate between applescript and modlunit
echo -e "\033]0;modlunit\007"
MODLUNIT="modlunit"

if test "x$1" != x ; then
	for i in "$@" ; do
		if test -f "$i" ; then
			cd `dirname "$i"`
			arg1=`basename "$i"`
			${MODLUNIT} "$i"
		else
			cd "$i"
			for j in *.mod ; do
				${MODLUNIT} "$j"
			done
		fi
	done
else
	echo "drag a folder or set of mod files onto the modlunit script"
	${MODLUNIT}
fi

echo "Press 'return' key to close"
read a




