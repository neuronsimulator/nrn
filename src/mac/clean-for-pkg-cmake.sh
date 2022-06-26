#!/usr/bin/env bash

if test "$1" = "" ; then
	echo "clean-for-pkg-cmake.sh needs a installation directory argument"
	exit
fi

NRN_INST="$1"
CPU=`uname -m`

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
	rm -f *.o *.c *.cpp makemod2c_inc

	for i in *.dylib ; do
		if test ! -L $i ; then
			strip -x $i
		fi
	done
fi


# get rid of useless iv installed files
(cd $NRN_INST ; rm -f tmp.tmp `find . -name Makefile\*`)
(cd $NRN_INST/lib ; rm -f tmp.tmp *.a)
(cd $NRN_INST/include ; rm -r -f Dispatch IV-2_6 IV-Mac IV-Win IV-X11 IV-look InterViews OS TIFF Unidraw)
