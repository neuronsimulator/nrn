#!/bin/sh
a=$1
if test "$a" = "" ; then
	a=.
fi
b=0
d=""
if test -d $a/.svn ; then
	b=`svnversion $a`
fi
echo "($b)"

