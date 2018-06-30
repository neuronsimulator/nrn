#!/bin/sh
# from "cvs update" to file in ftp site versions/alpha directory

NSRC=$HOME/neuron/nrn

cd $NSRC
oldver=""
if test -f oldver ; then
	oldver="`cat oldver`"
fi
cvs update -d
newver="`sh nrnversion.sh commit`"
if test "$oldver" = "$newver" ; then
	exit
fi

./build.sh

os="`sh config.guess|sed 's/^\([^-]*\)-\([^-]*\)-\([a-z]*\).*$/\3/'`"
echo $os
if test "$os" = "linux" ; then
	NOBJ=$HOME/neuron/nrnrpm
	cd $NOBJ
$NSRC/configure --prefix=/usr/local/nrn --with-iv=/usr/local/iv \
  --srcdir=$NSRC --enable-rpm-rules --with-nrnjava --disable-static
	make
	make dist
	make alphadist
	sudo make install
	sudo make rpm
	make rpmdist
fi

if test "$os" = "darwin" ; then
	NOBJ=$HOME/neuron/nrncarbon
	NVER="`sh $NSRC/nrnversion.sh`"
	IDIR=/Applications/NEURON-$NVER
	cd $NOBJ
	$NSRC/configure --prefix=$IDIR/nrn --srcdir=$NSRC \
                 --with-iv=$IDIR/iv --with-nrnjava --enable-carbon
        make
        make install
        make after_install
        make dmg
        make alphadist
fi

if test "$os" = "cygwin" ; then
	./configure --prefix=`pwd` --with-nrnjava
	make
	make mswin
	make alphadist
fi

echo "$newver" > $NSRC/oldver

