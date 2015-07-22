#!/bin/sh
# install from the build directories to the mswin destination
# uses the classical positions of files
# must be launched from a mingw terminal

if false ; then
echo "top_srcdir $top_srcdir"
echo "top_builddir $top_builddir"
echo "marshal_dir $marshal_dir"
echo "ivbindir $ivbindir"
echo "host_cpu $host_cpu"
echo "CC $CC"
exit 0
fi

if test "$top_srcdir" = "" ; then
	echo "instalmingw.sh should be executed at top level with make mswin"
else
	S=`(cd $top_srcdir ; readlink -f .)`
	export S
fi
if test "$top_builddir" = "" ; then
	echo "instalmingw.sh should be executed at top level with make mswin"
else
	B=`(cd $top_builddir ; readlink -f .)`
	export B
fi
if test "$marshal_dir" = "" ; then
	echo "instalmingw.sh should be executed at top level with make mswin"
else
	if test ! -d $marshal_dir ; then
		mkdir $marshal_dir
	fi
	D=`(cd $marshal_dir ; readlink -f .)`
	if test $host_cpu = x86_64 ; then
		BIT=64
		D=$D/nrn
		DB=$D/bin
	else
		BIT=32
		D=$D/nrn
		DB=$D/bin
	fi
	export D
	export DB
fi

H=$HOME
if test "$host_cpu" = x86_64 ; then
	SYS=/mingw/x86_64-w64-mingw32
else
	SYS=/mingw
fi

if test "$ivbindir" = "" ; then
	echo "instal.sh should be executed at top level with make mswin"
	exit 1
fi
echo "MSWIN install from $S and $B to $D"

set -x

if true ; then # false means skip the entire marshaling of nrn

MG=$D/mingw

rm -r -f $D
mkdir $D
mkdir $DB
mkdir $D/lib

# copy and strip the various executables we built
cp $B/src/nrniv/mos2nrn.exe $DB/mos2nrn.exe
strip $DB/mos2nrn.exe
cp $B/src/nrniv/neuron.exe $DB/neuron.exe
strip $DB/neuron.exe
cp $B/src/mswin/nrniv.exe $DB/nrniv.exe
strip $DB/nrniv.exe
if test -f $B/src/mswin/nrniv_enthought.exe ; then
  cp $B/src/mswin/nrniv_enthought.exe $DB/nrniv_enthought.exe
  strip $DB/nrniv_enthought.exe
  cp /c/Python27/Microsoft.VC90.CRT.manifest $DB
  cp /c/Python27/msvcr90.dll $DB
fi
cp $B/src/mswin/nrniv.dll $DB/nrniv.dll
strip $DB/nrniv.dll
cp $B/src/mswin/libnrnmpi.dll $DB/libnrnmpi.dll
strip $DB/libnrnmpi.dll
#will move hocmodule to lib/python/neuron/hoc.pyd after lib/python is created
cp $B/src/mswin/hocmodule.dll $DB/hocmodule.dll
strip $DB/hocmodule.dll
cp $B/src/mswin/libnrnpython1013.dll $DB
strip $DB/libnrnpython1013.dll
cp $ivbindir/libIVhines-3.dll $DB/libIVhines-3.dll
strip $DB/libIVhines-3.dll

cp $H/neuron/termcap-1.3.1/libtermcap.dll $DB
strip $DB/libtermcap.dll
cp $H/neuron/readline-6.2/shlib/libhistory6.dll $DB
strip $DB/libhistory6.dll
cp $H/neuron/readline-6.2/shlib/libreadline6.dll $DB
strip $DB/libreadline6.dll
cp $H/neuron/regex-0.12/libregex.dll $DB
strip $DB/libregex.dll

if test "$host_cpu" = x86_64 ; then
cp $H/pthreads-20100604/mingw64/bin/pthreadGC2-w64.dll $DB
strip $DB/pthreadGC2-w64.dll
mkdir $D/gccinc
cp $H/pthreads-20100604/mingw64/x86_64-w64-mingw32/lib/*.a $DB
cp $H/pthreads-20100604/mingw64/x86_64-w64-mingw32/include/*.h $D/gccinc
else
cp $SYS/bin/pthreadGC2.dll $DB
strip $DB/pthreadGC2.dll
fi

if test "$host_cpu" = x86_64 ; then
cp $SYS/lib/libgcc_s_sjlj-1.dll $DB
strip $DB/libgcc_s_sjlj-1.dll
cp $SYS/lib/libstdc++-6.dll $DB
strip $DB/libstdc++-6.dll
else
cp $SYS/bin/libgcc_s_dw2-1.dll $DB
strip $DB/libgcc_s_dw2-1.dll
cp $SYS/bin/libstdc++-6.dll $DB
strip $DB/libstdc++-6.dll
fi

if test -f $B/src/nmodl/.libs/nocmodl.exe ; then
	cp $B/src/nmodl/.libs/nocmodl.exe $DB
	cp $B/src/modlunit/.libs/modlunit.exe $DB
else
	cp $B/src/nmodl/nocmodl.exe $DB
	cp $B/src/modlunit/modlunit.exe $DB
fi
strip $DB/nocmodl.exe
strip $DB/modlunit.exe

if test "$LTCC" = "" ; then
	LTCC=$CC
fi

# extract enough mingw stuff so mknrndll will work.
#This adds 13MB to the installer.
unzip -d $D -o $S/../mingw${BIT}_nrndist.zip

# in case this is an mpi version distribute the appropriate administrative tools.
if test "$PARANEURON"="yes" ; then
	mpiinstalled=/c/ms-mpi
	# gforker
	cp $mpiinstalled/bin/mpiexec.exe $DB
	cp $mpiinstalled/bin/smpd.exe $DB
	#cp /c/Windows/System32/mpich2mpi.dll $DB
	#cp $S/../mpich2mpi.dll $DB
	if test $host_cpu = x86_64 ; then
		cp $mpiinstalled/lib/x64/msmpi.dll $DB
	else
		cp $mpiinstalled/lib/x86/msmpi.dll $DB
	fi
	# and make the basic tests available
	for i in test0.hoc test0.py ; do
		cp $S/src/parallel/$i $D
		unix2dos $D/$i
	done
fi

if false ; then #to foo one
if test -f "$S/src/nrnjava/neuron.jar" ; then
	mkdir $D/classes
	cp $S/src/nrnjava/*.jar $D/classes
fi
fi #foo one

cp $S/src/mswin/*.ico $DB

# copy the dos formatted files to the classical positions.
# use zip to do the translation from unix to dos format

Z=$B/d2ufiles.zip

if true ; then
cd $S/share
rm -f $Z
zip -l -r $Z examples lib demo -x \*.svn\*
zip -d $Z \*,v \*.svn\* \*.in \*Makefile\* \*.o \*.c \*.dll \*/auditscripts\*
unzip -d $D -o $Z 
rm $Z
cd $B/share
rm -f $Z
zip -l $Z lib/nrn.defaults
unzip -d $D -o $Z 
rm $Z
mv $DB/hocmodule.dll $D/lib/python/neuron/hoc.pyd
fi

if true ; then
cd $S
rm -f $Z
zip -l $Z src/oc/*.h src/nrnoc/*.mod src/nrnoc/*.h src/scopmath/*.h
(cd $B ; zip -l $Z src/oc/*.h src/nrnoc/*.mod src/nrnoc/*.h src/scopmath/*.h)
unzip -d $D -o $Z
rm $Z
fi

if true ; then
cd $S/src/mswin
cp bin/mknrndll bin/neurondemo bin/nrngui $DB
rm -f $Z
zip -l $Z notes.txt
#do the lib shell scripts in unix format
zip $Z lib/*.sh lib/*.sed
#do the specified unix bin shell scripts in unix format
cd $S
cp bin/mkthreadsafe bin/nrnpyenv.sh bin/set_nrnpyenv.sh bin/sortspike $DB
unzip -d $D -o $Z

cd $B/src/mswin
rm -f $Z
#do the lib shell scripts in unix format
zip $Z lib/*.mak
unzip -d $D -o $Z
rm $Z
fi

if true ; then
cd $S/src/mswin
mv $D/lib/nrn.defaults $D/lib/nrn.def
fi

#fix the path to make
cd $D/lib
sed 's;\$N/bin/make;make;' mknrndl2.sh > temp
mv temp mknrndl2.sh

#for neurondemo
cd $D/demo/release
export N=$D
export PATH="$DB:$D/mingw/bin:$PATH"
sh $D/lib/mknrndl2.sh
rm *.o *.c
if test ! -f nrnmech.dll ; then
  echo 'could not build nrnmech.dll'
  exit 1;
fi

# what the shortcuts should look like

#mknrndll icon shortcut
# c:\nrn\bin\nrniv.exe c:/nrn/lib/hoc/mknrndll.hoc
# start in c:

#nrngui icon shortcut
# c:\nrn\bin\neuron.exe c:/nrn/lib/hoc/nrngui.hoc
# start in c:

#neurondemo icon shortcut
# c:\nrn\bin\neuron.exe -dll c:/nrn/demo/release/nrnmech.dll demo.hoc
# start in c:\nrn\demo

fi # end of nrn marshaling

if true ; then # false means skip marshaling of html

hparent=$S/..
if false ; then
cd $hparent
scp hines@www.neuron.yale.edu:/home/htdocs/neuron/static/docs/nrnhelp.zip .
mkdir html
cd html
unzip ../nrnhelp.zip
fi
if test -d "$hparent/html" ; then
	cd $hparent
	rm html.zip
	zip -r html.zip html -x \*.svn\*
	rm -r -f $marshal_dir/html
	unzip -d $marshal_dir html.zip
	rm html.zip
fi

fi # end of html marshaling

set +v
echo "Will now complete the creation of the installer by launching"
echo "    $B/src/mswin/nrnsetup.nsi ."
echo " The installer will be located in $S/src/mswin/nrnxxsetup.exe"

if true ; then #make the nrnsetup.exe
cd $B/src/mswin
#c:/Program\ Files/NSIS/makensisw nrnsetup.nsi
if test $host_cpu = x86_64 ; then
c:/Program\ Files\ \(x86\)/NSIS/makensis nrnsetupmingw.nsi
else
c:/Program\ Files/NSIS/makensis nrnsetupmingw.nsi
fi
echo " The installer is located in $S/src/mswin/nrnxxsetup.exe"
fi
