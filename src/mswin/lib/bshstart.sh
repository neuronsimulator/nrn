#!/bin/sh
N=`pwd`
PATH=$N/bin:$PATH
export N
export PATH
#to avoid bash warning, create /tmp if it does not exist
if test ! -e /tmp ; then
	a=`cygpath --mixed /tmp`
	mkdir -p $a
	if test -d /tmp ; then
		echo "Notice: created $a to avoid bash shell warning in future."
	else
		echo "Notice: could not create $a , the bash shell warning can be ignored."
	fi
fi
cd c:/
