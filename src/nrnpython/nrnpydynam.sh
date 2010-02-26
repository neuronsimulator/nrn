#!/bin/sh

echo "nrnpydynam.sh $1 $2 $3"

srcdir="$1"
depdir="$2"
host="$3"

# Specific for Hines machines which build distributions
case "$host" in
  i686*linux-gnu)
	pyver='23 24 25 26'
	inc23=-I/home/hines/python/python23/include/python2.3
	inc24=-I/home/hines/python/python24/include/python2.4
	inc25=-I/home/hines/python/python25/include/python2.5
	inc26=-I/home/hines/python/python26/include/python2.6
	;;
  x86_64*linux-gnu)
	pyver='23 24 25 26 30'
	inc23=-I/home/hines/python/python23/include/python2.3
	inc24=-I/home/hines/python/python24/include/python2.4
	inc25=-I/home/hines/python/python25/include/python2.5
	inc26=-I/home/hines/python/python26/include/python2.6
	inc30=-I/home/hines/python/python30/include/python3.0
	;;
  *darwin*)
	pyver='23 24 25 26'
	inc23=-I/usr/include/python2.3
	inc24=-I/Library/Frameworks/Python.framework/Versions/2.4/include/python2.4
	inc25=-I/Library/Frameworks/Python.framework/Versions/2.5/include/python2.5
	inc26=-I/Library/Frameworks/Python.framework/Versions/2.6/include/python2.6
	;;  
esac

for d in $pyver ; do
	if ! test -d npy$d ; then
		mkdir npy$d
	fi
	if ! test -d npy$d/$depdir ; then
		mkdir npy$d/$depdir
		cp nrnpython/$depdir/* npy$d/$depdir
	fi
	a=inc$d
	sed " s,libnrnpython,libnrnpython$d,g
		/^INCLUDES =/s,I\. ,I. -I../nrnpython ,
		/^NRNPYTHON_INCLUDES =/s,.*,NRNPYTHON_INCLUDES = ${!a},
	" nrnpython/Makefile > npy$d/Makefile
done
