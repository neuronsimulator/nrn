#!/bin/bash
# must be bash because of indirect expansion ${!a} below.

echo "nrnpydynam.sh $1 $2 $3"

srcdir="$1"
depdir="$2"
host="$3"

# Versions we would like to be able to dynamically load
pyver='24 25 26 27'

# Specific for Hines machines which build distributions
case "$host" in
  i686*linux-gnu)
	inc24=-I${HOME}/python/include/python2.4
	inc25=-I${HOME}/python/include/python2.5
	inc26=-I${HOME}/python/include/python2.6
	inc27=-I${HOME}/python/include/python2.6
	;;
  x86_64*linux-gnu)
	inc24=-I${HOME}/python/include/python2.4
	inc25=-I${HOME}/python/include/python2.5
	inc26=-I${HOME}/python/include/python2.6
	inc27=-I${HOME}/python/include/python2.7
	;;
  *darwin*)
	inc26=-I${HOME}/python/include/python2.6
	inc27=-I${HOME}/python/include/python2.7
	;;  
esac

for d in $pyver ; do
	if ! test -d npy$d ; then
		mkdir npy$d
	fi
	a=inc$d
	a=${!a}
    if test "$a" != "" ; then
	if ! test -d npy$d/$depdir ; then
		mkdir npy$d/$depdir
		cp nrnpython/$depdir/* npy$d/$depdir
	fi
	sed " s,libnrnpython,libnrnpython$d,g
		/^INCLUDES =/s,I\. ,I. -I../nrnpython ,
		/^NRNPYTHON_INCLUDES =/s,.*,NRNPYTHON_INCLUDES = $a,
	" nrnpython/Makefile > npy$d/Makefile
    else
	# Create a Makefile that does nothing
	echo "
all:
	true
install:
	true
clean:
	true
"	> npy$d/Makefile
    fi
done
