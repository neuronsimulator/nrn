#/bin/sh
# from svn update to alpha on ftp site for linux, mswin, and mac osx
# usage:sh mkalpha.sh
#	sh mkalpha.sh force
# with force, will continue even if an ftp distribution file matches the
# svn updated version number

# must execute in the top level source directory
NSRC=`pwd`
export NSRC
cd $NSRC

lastalpha="`sh nrnversion.sh 2`"
hg pull
hg update
if test $? != 0 ; then
	exit 1
fi
srcdir=$NSRC/src/nrnoc
sh $NSRC/hg2nrnversion_h.sh > $srcdir/nrnversion.h.tmp
cmp $srcdir/nrnversion.h $srcdir/nrnversion.h.tmp || cp $srcdir/nrnversion.h.tmp $srcdir/nrnversion.h
rm $srcdir/nrnversion.h.tmp

if test ! -f config.guess ; then
	sh build.sh
fi

ostype="`sh config.guess | sed 's/^\([^-]*\)-\([^-]*\)-\([A-Za-z]*\).*/\3/'`"
ver="`sh nrnversion.sh 2`"

a=1
old="`ssh hines@www.neuron.yale.edu 'cd /home/htdocs/ftp/neuron/versions/alpha; ls *'$ver'*'`"

case "$ostype" in
	cygwin) echo "$old" |grep 'setup' ; a=$? ;;
	linux) host="`./config.guess`"
		echo "$old" |grep "$host" ; a=$? ;;
	darwin) host="`./config.guess`"
		echo "$old" |grep "$host" ; a=$? ;;
esac

if test "$a" = "0" ; then
	echo "up to date"
	if test "$1" != "force" ; then
		exit 0
	fi
fi

currentalpha="`sh nrnversion.sh 2`"
base="`sh nrnversion.sh global`"
type="`sh nrnversion.sh type`"
echo "$old"
echo "$ostype old="$lastalpha" new=$currentalpha"

./build.sh
NVER="`sh nrnversion.sh`"

#mswin
if test "$ostype" = "cygwin" ; then
NOBJ=$HOME/neuron/nrn${type}setup
if test ! -d $NOBJ ; then
	mkdir $NOBJ
fi
cd $NOBJ
$NSRC/configure --prefix=`pwd` --with-nrnpython --with-paranrn \
  --with-readline=yes
make
if test $? != 0 ; then
	echo "make failed"
	exit 1
fi
newver=`src/nrniv/nrniv --version | sed 's/[^(]*.*:\(.*\)).*/\1/'`
if "$newver" != "$base" ; then
	exit 1
fi
make mswin
make alphadist
fi

#linux
if test "$ostype" = "linux" ; then
NOBJ=$HOME/neuron/nrn${type}rpm
if test ! -d $NOBJ ; then
	mkdir $NOBJ
fi
cd $NOBJ
which javac ; a=$?
jarg=""
a=1
if test "$a" = 0 ; then
	jarg='--with-nrnjava'
fi
$NSRC/configure --prefix=/usr/local/nrn --with-iv=/usr/local/iv \
	--srcdir=$NSRC --enable-rpm-rules --disable-static \
	--with-readline=yes --with-nrnpython=dynamic --with-paranrn=dynamic $jarg
make
if test $? != 0 ; then
	echo "make failed"
	exit 1
fi
newver=`src/nrniv/nrniv --version | sed 's/[^(]*.*:\(.*\)).*/\1/'`
if "$newver" != "$base" ; then
	exit 1
fi
#if test "$a" = 0 ; then
	make dist
	make alphadist
#fi
sudo $HOME/bin/make_install_rpm # make install; make rpm
make rpmdist
fi

#mac os x
if test "$ostype" = "darwin" ; then
#NOBJ=$HOME/neuron/nrn${type}carbon
NOBJ=$HOME/neuron/nrn${type}x11
IDIR=/Applications/NEURON-$NVER
if test ! -d $NOBJ ; then
	mkdir $NOBJ
fi
cd $NOBJ
#$NSRC/configure --prefix=$IDIR/nrn --srcdir=$NSRC \
#	--with-iv=$IDIR/iv --enable-carbon --with-nrnpython \
#	PYLIB=-lpython PYLIBLINK=-lpython --enable-UniversalMacBinary
#$NSRC/configure --prefix=$IDIR/nrn --srcdir=$NSRC \
#  --with-iv=$IDIR/iv --enable-carbon --with-nrnpython=dynamic \
#  'CFLAGS=-arch i386 -g -O2' 'CXXFLAGS=-arch i386 -g -O2'
$NSRC/configure --prefix=$IDIR/nrn  \
  --with-iv=$IDIR/iv --with-paranrn=dynamic --with-nrnpython=dynamic

make clean
make
if test $? != 0 ; then
	echo "make failed"
	exit 1
fi
make install
make after_install
host_cpu="`echo $host | sed 's/-.*//'`"
if test -d "$IDIR/nrn/umac" ; then
	host_cpu=umac
fi
newver=`${IDIR}/nrn/${host_cpu}/bin/nrniv --version | sed 's/[^(]*.*:\(.*\)).*/\1/'`
if test "$newver" != "$base" ; then
	exit 1
fi
make pkg
make alphadist
fi
