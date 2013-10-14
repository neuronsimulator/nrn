#!/bin/sh
# how to build from just the cvs sources
# takes us only to the distribution level.
# must still run a ./configure commmand (see examples after autoconf command)
########################################
########################################
# the following commands must be executed in this directory

aclocal -I m4

ltarg="-i"
if test -f "`which glibtoolize`" ; then
	ltver=`glibtoolize --version | sed -n '1s/.* \([0-9]\).*/\1/p'`
	if test "${ltver}" = 1 ; then ltarg="" ; fi
	echo "glibtoolize -c -f $ltarg"
	glibtoolize -c -f $ltarg
else
	ltver=`libtoolize --version | sed -n '1s/.* \([0-9]\).*/\1/p'`
	if test "${ltver}" = 1 ; then ltarg="" ; fi
	echo "libtoolize -c -f $ltarg"
	libtoolize -c -f $ltarg
fi
    
autoheader

#autoheader constructs config.h.in. From that, construct nrnconf.h.in
# and nmodlconf.h.in. The former is for binaries executed on the host machine
# and the latte is for binaryies executed on the build machine
sed '/Remove from nrnconf\.h\.in/,/^$/d' config.h.in > temp.tmp
! cmp -s temp.tmp nrnconf.h.in && cp temp.tmp nrnconf.h.in
! cmp -s temp.tmp nmodlconf.h.in && cp temp.tmp nmodlconf.h.in
rm temp.tmp

automake --add-missing --copy
sh extend_depcomp.sh
autoconf

# at this point the rest of the build process is identical as if you
# had just extracted a nrn-5.4.tar.gz file

# the configure, make, make install may be executed in this directory
# or a completely different build directory. Examples for this directory
# are:
# if Interviews was installed in ../iv and you want java
#	./configure --prefix=`pwd` --with-nrnjava
# if InterViews was installed in $HOME/iv-15
#	./configure --prefix=`pwd` --with-iv=$HOME/iv-15
# if this directory is $NSRC, InterViews was installed in $IV
# and you want to build in $NOBJ then cd to $NOBJ and use at least
#	$NSRC/configure --prefix=`pwd` --with-iv=$IV --srcdir=$NSRC

# make
# make install

###########
