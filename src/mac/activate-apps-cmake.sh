#!/bin/sh

# This started as a copy of after-install.sh and is modified to work in the
# context subsequent to a cmake build 'make install'. The main differences
# are that icons and apps are installed in CMAKE_INSTALL_PREFIX,
# that the $CPU is not used for nrn/bin and nrn/lib, and that
# instead of there being an iv folder at the same level as nrn, idraw is
# in CMAKE_INSTALL_PREFIX/bin. For distributions, it is presumed that iv
# is a submodule and that
# IV_ENABLE_X11_DYNAMIC=ON
# Also the autotools install called launch_inst.sh and copied some
# launch scripts. That is now done here as well.

if test "$1" = "" ; then
	echo "after-cmake-install needs a host_cpu argument"
	exit
fi

if test "$2" = "" ; then
	echo "after-cmake-install needs a installation directory argument"
	exit
fi

CPU=$1
prefix=$2
srcdir=$3
ivlibdir=$4
export CPU

# Equivalent to install from the Makefile.am
S="modlunit.sh mknrndll.sh nrngui.sh neurondemo.sh mos2nrn.sh"
cp $srcdir/src/mac/macnrn.term $prefix/bin/macnrn.term
for i in $S ; do
  sed "s/^CPU.*/CPU=\"$CPU)\"/" < $srcdir/src/mac/$i > temp
  mv temp $prefix/bin/$i
  chmod 755 $prefix/bin/$i
done
sed "s,\.\./iv/x86_64/,," < $srcdir/src/mac/idraw.sh > $prefix/bin/idraw.sh
chmod 755 $prefix/bin/idraw.sh
sh $srcdir/src/mac/launch_inst_cmake.sh "." "$prefix" "$srcdir/src/mac"

NSRC="$3"
export NSRC
NVER="`sh $srcdir/nrnversion.sh`"
NDIR="$prefix"
export NDIR
N="$prefix"
IV=`echo "$prefix"|sed 's/nrn$/iv/'` # Does not exist if iv is a submodule

# activate macnrn.term and the app bundles
# process prefix into a list that walks the path
# escaped " needed in case of spaces
a="$prefix"
lst="\"`basename \"$a\"`\""
a="`dirname \"$a\"`"
while test "$a" != "/" ; do
  lst="$lst of folder \"`basename \"$a\"`\""
  a="`dirname \"$a\"`"
done

echo "$lst"

osascript -e 'tell application "Finder"'\
 -e 'activate'\
 -e 'make new Finder window'\
 -e 'set target of Finder window 1 to folder '"$lst"' of startup disk'\
 -e 'set target of Finder window 1 to folder "bin" of folder '"$lst"' of startup disk'\
 -e 'end tell'

# force rebuild of the neurondemo
DEMO="${N}/share/nrn/demo"
rm -f -r ${DEMO}/neuron ${DEMO}/release/${CPU}
$prefix/bin/neurondemo << here
quit()
here
