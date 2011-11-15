#!/bin/bash
# must be bash because of indirect expansion ${!a} below.

echo "nrnpydynam.sh $1 $2 $3"

srcdir="$1"
depdir="$2"
host="$3"

# Specific for Hines machines which build distributions
case "$host" in
  i686*linux-gnu)
	pyver='24 25 26 27'
	inc24=-I/home/hines/python/include/python2.4
	inc25=-I/home/hines/python/include/python2.5
	inc26=-I/home/hines/python/include/python2.6
	inc27=-I/home/hines/python/include/python2.6
	;;
  x86_64*linux-gnu)
	pyver='24 25 26 27'
	inc24=-I/home/hines/python/include/python2.4
	inc25=-I/home/hines/python/include/python2.5
	inc26=-I/home/hines/python/include/python2.6
	inc27=-I/home/hines/python/include/python2.7
	;;
  *darwin*)
	pyver='24 25 26 27'
	inc24=-I/Users/michaelhines/python/include/python2.4
	inc25=-I/Users/michaelhines/python/include/python2.5
	inc26=-I/Users/michaelhines/python/include/python2.6
	inc27=-I/Users/michaelhines/python/include/python2.7
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
