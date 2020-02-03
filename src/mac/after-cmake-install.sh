#!/bin/sh

# This started as a copy of after-install.sh and is modified to work in the
# context subsequent to a cmake build 'make install'. The main differences
# are that the $CPU is not used for nrn/bin and nrn/lib, and that
# instead of there being an iv folder at the same level as nrn, idraw is
# in nrn/bin. It is presumed that iv is a submodule and that
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
NDIR="NEURON-$NVER"
export NDIR
N="$prefix"
IV=`echo "$prefix"|sed 's/nrn$/iv/'` # Does not exist if iv is a submodule

# activate macnrn.term and the app bundles
osascript -e 'tell application "Finder"'\
 -e 'activate'\
 -e 'make new Finder window'\
 -e 'set target of Finder window 1 to folder "'$NDIR'" of folder "Applications" of startup disk'\
 -e 'set target of Finder window 1 to folder "bin" of folder "nrn" of folder "'$NDIR'" of folder "Applications" of startup disk'\
 -e 'end tell'

#strip the dylibs
cd ${N}/lib
for i in *.dylib ; do
	if test ! -L $i ; then
		strip -x $i
	fi
done

#remove irrelevant executables
cd ${N}/bin
rm -f iclass idemo

# and strip the others
for i in idraw modlunit nocmodl nrniv ; do
	strip -x $i
done
if test "$carbon" = "yes" ; then
	strip -x nrniv.app/Contents/MacOS/nrniv
fi

# rxdtests/correct_data size is 46M compared to total 78M
rm -r -f ${N}/share/nrn/lib/python/neuron/rxdtests/correct_data

# force rebuild of the neurondemo and clean it up
DEMO="${N}/share/nrn/demo"
rm -f -r ${DEMO}/neuron ${DEMO}/release/${CPU}
./neurondemo << here
quit()
here
if cd ${N}/share/nrn/demo/release/${CPU} ; then
	rm -f *.o *.c *.mod

	for i in *.dylib ; do
		if test ! -L $i ; then
			strip -x $i
		fi
	done
	# make the -dll position independent
	cd ../..
	sed '/\-dll/s,\-dll "/App.*/share/nrn,-dll "${NRNHOME}/share/nrn,' neuron > ntmp
	mv ntmp neuron
	chmod 755 neuron
fi


# get rid of useless iv installed files
(cd $N ; rm -f tmp.tmp `find . -name Makefile\*`)
(cd $N/lib ; rm -f tmp.tmp *.a)
(cd $N/include ; rm -r -f Dispatch IV-2_6 IV-Mac IV-Win IV-X11 IV-look InterViews OS TIFF Unidraw)

vv=`sw_vers -productVersion|sed 's/.*\.\(.*\)\..*/\1/'`
#if test $vv -gt 6 ; then
if false ; then

# change nrniv install_name for dylibs so nrniv,etc works at any location
ff="$N/bin/nrniv"
f=`otool -L "$N/bin/nrniv" | sed -n "s,.*$N/lib/\(.*dylib\).*,\
-change $N/lib\1 @executable_path/../lib/\1,p"`
install_name_tool $f "$ff"

#this one needs to be done from nrnivmodl, same as all the libnrnmech.so
if false ; then
ff="$N/share/nrn/demo/release/x86_64/libnrnmech*.dylib"
f=`otool -L $ff | sed -n "s,.*$N/lib/\(.*dylib\).*,\
-change $N/lib/\1 @executable_path/../lib/\1,p"`
install_name_tool $f $ff
fi

# neurondemo for libnrnmech.so needs to follow the install location.
f=/tmp/neuron$$
sed '
 s,..NRNHOME./share/nrn/demo,@executable_path/../share/nrn/demo,
' $N/share/nrn/demo/neuron > $f
chmod 755 $f
mv $f $N/share/nrn/demo/neuron

fi
