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
NRN_INSTALL=$2
NRN_SRC=$3
ivlibdir=$4
export CPU
export NRN_SRC
NRN_VERSION="`sh $NRN_SRC/nrnversion.sh`"

# Equivalent to install from the Makefile.am
S="modlunit.sh mknrndll.sh nrngui.sh neurondemo.sh mos2nrn.sh"
cp $NRN_SRC/src/mac/macnrn.term $NRN_INSTALL/bin/macnrn.term
for i in $S ; do
  cp $NRN_SRC/src/mac/$i $NRN_INSTALL/bin/$i
  chmod 755 $NRN_INSTALL/bin/$i
done
sed "s,\.\./iv/x86_64/,," < $NRN_SRC/src/mac/idraw.sh > $NRN_INSTALL/bin/idraw.sh
chmod 755 $NRN_INSTALL/bin/idraw.sh

sh $NRN_SRC/src/mac/launch_inst_cmake.sh "$NRN_INSTALL" "$NRN_SRC"

# activate macnrn.term and the app bundles
# process NRN_INSTALL into a list that walks the path
# escaped " needed in case of spaces
a="$NRN_INSTALL"
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
DEMO="${NRN_INSTALL}/share/nrn/demo"
rm -f -r ${DEMO}/neuron ${DEMO}/release/${CPU}
$NRN_INSTALL/bin/neurondemo << here
quit()
here
