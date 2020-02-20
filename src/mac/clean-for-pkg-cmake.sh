#!/bin/sh

if test "$1" = "" ; then
	echo "clean-for-pkg-cmake.sh needs a installation directory argument"
	exit
fi

NRN_INSTALL="$1"
CPU=x86_64

#strip the dylibs
cd ${NRN_INST}/lib
for i in *.dylib ; do
	if test ! -L $i ; then
		strip -x $i
	fi
done

#remove irrelevant executables
cd ${NRN_INST}/bin
rm -f iclass idemo

# and strip the others
for i in idraw modlunit nocmodl nrniv ; do
	strip -x $i
done

# rxdtests/correct_data size is 46M compared to total 78M
rm -r -f ${NRN_INST}/share/nrn/lib/python/neuron/rxdtests/correct_data

# clean up the neurondemo
DEMO="${NRN_INST}/share/nrn/demo"
if cd ${NRN_INST}/share/nrn/demo/release/${CPU} ; then
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
(cd $NRN_INST ; rm -f tmp.tmp `find . -name Makefile\*`)
(cd $NRN_INST/lib ; rm -f tmp.tmp *.a)
(cd $NRN_INST/include ; rm -r -f Dispatch IV-2_6 IV-Mac IV-Win IV-X11 IV-look InterViews OS TIFF Unidraw)

vv=`sw_vers -productVersion|sed 's/.*\.\(.*\)\..*/\1/'`
#if test $vv -gt 6 ; then
if false ; then

# change nrniv install_name for dylibs so nrniv,etc works at any location
ff="$NRN_INST/bin/nrniv"
f=`otool -L "$NRN_INST/bin/nrniv" | sed -n "s,.*$NRN_INST/lib/\(.*dylib\).*,\
-change $NRN_INST/lib\1 @executable_path/../lib/\1,p"`
install_name_tool $f "$ff"

#this one needs to be done from nrnivmodl, same as all the libnrnmech.so
if false ; then
ff="$NRN_INST/share/nrn/demo/release/x86_64/libnrnmech*.dylib"
f=`otool -L $ff | sed -n "s,.*$NRN_INST/lib/\(.*dylib\).*,\
-change $NRN_INST/lib/\1 @executable_path/../lib/\1,p"`
install_name_tool $f $ff
fi

# neurondemo for libnrnmech.so needs to follow the install location.
f=/tmp/neuron$$
sed '
 s,..NRNHOME./share/nrn/demo,@executable_path/../share/nrn/demo,
' $NRN_INST/share/nrn/demo/neuron > $f
chmod 755 $f
mv $f $NRN_INST/share/nrn/demo/neuron

fi
