#!/bin/sh
# install from the build directories to the mswin destination
# uses the classical positions of files
# must be launched from a mingw terminal
set -ex

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
mkdir $D/tmp

# copy the various executables we built
cp $B/src/nrniv/nrnbinstr.exe $DB/nrnbinstr.exe
cp $B/src/nrniv/mos2nrn.exe $DB/mos2nrn.exe
cp $B/src/nrniv/neuron.exe $DB/neuron.exe
cp $B/src/mswin/nrniv.exe $DB/nrniv.exe
if test -f $B/src/mswin/nrniv_crt90.exe ; then
  # overwrite (avoids R6034 error from python2.7 on certain imports)
  cp $B/src/mswin/nrniv_crt90.exe $DB/nrniv.exe
  cp /e/Python27/Microsoft.VC90.CRT.manifest $DB
  cp /e/Python27/msvcr90.dll $DB
fi
cp $B/src/mswin/nrniv.dll $DB/nrniv.dll
cp $B/src/mswin/libnrnmpi.dll $DB/libnrnmpi.dll
#will move hocmodule to lib/python/neuron/hoc.pyd after lib/python is created
cp $B/src/mswin/hocmodule*.dll $DB
cp $B/src/mswin/libnrnpython*.dll $DB
cp $B/src/mswin/librxdmath.dll $DB
cp $ivbindir/libIVhines-3.dll $DB/libIVhines-3.dll

if test -f $B/src/nmodl/.libs/nocmodl.exe ; then
	cp $B/src/nmodl/.libs/nocmodl.exe $DB
	cp $B/src/modlunit/.libs/modlunit.exe $DB
else
	cp $B/src/nmodl/nocmodl.exe $DB
	cp $B/src/modlunit/modlunit.exe $DB
fi

if test "$LTCC" = "" ; then
	LTCC=$CC
fi

# extract enough mingw stuff so mknrndll will work.
#This adds 22MB to the installer.
sh $S/mingw_files/nrnmingwenv.sh $D
#mingw64_nrndist created using nrn/mingw_files/nrnmingwenv.sh
#unzip -d $D -o $S/../mingw${BIT}_nrndist.zip
#cp $S/../pthreadGC2-w64.dll $DB

if false ; then
# copy some useful tools (now done my mingw_files/nrnmingwenv.sh)
for i in \
  basename, bash cat cp dirname echo find grep ls make mintty mkdir mv \
  rebase rm sed sh sort unzip zip \
  cygcheck
do
  cp /usr/bin/$i.exe $D/bin
done    
fi

# Determine what msys64 dlls are needed by the bin programs and copy them
(cd $DB
  rm -f temp.tmp

  for i in *.exe ; do
    cygcheck ./$i | sed 's/^ *//' >> temp.tmp
  done
)
  
sort $DB/temp.tmp | uniq | grep 'msys64' | sed 's,\\,/,g' > $DB/temp2.tmp
for i in `cat $DB/temp2.tmp` ; do
 echo $i
 cp $i $DB
done
     
rm $DB/temp.tmp
rm $DB/temp2.tmp
    
# reduce size of bin folder
(cd $DB ; strip *.exe *.dll)

# in case this is an mpi version distribute the appropriate administrative tools.
if test "$PARANEURON"="yes" ; then
	mpiinstalled=/c/ms-mpi
	# gforker
	cp $mpiinstalled/bin/mpiexec.exe $DB
	cp $mpiinstalled/bin/smpd.exe $DB
	#cp /c/Windows/System32/mpich2mpi.dll $DB
	#cp $S/../mpich2mpi.dll $DB
	if test $host_cpu = x86_64 ; then
		if test -f $mpiinstalled/lib/x64/msmpi.dll ; then
			cp $mpiinstalled/lib/x64/msmpi.dll $DB
		else
			zz=`cygcheck $DB/libnrnmpi.dll | grep msmpi.dll`
			if test "$zz" != "" ; then
				cp $zz $DB
			fi
		fi
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
zip -l $Z lib/nrn.defaults lib/nrnunits.lib
unzip -d $D -o $Z 
rm $Z
for f in $DB/hocmodule*.dll ; do
  x=`echo $f | sed "s/.*hocmodule\([0-9]*\)\.dll/\1/"`
  mv $f $D/lib/python/neuron/hoc${x}.pyd
done
set +e
cp $B/share/lib/python/neuron/rxd/geometry3d/*.pyd $D/lib/python/neuron/rxd/geometry3d
cp $B/share/lib/python/neuron/crxd/geometry3d/*.pyd $D/lib/python/neuron/crxd/geometry3d
set -e
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
cp bin/mknrndll $DB/nrnivmodl
cp bin/nrnivmodl.bat $DB/nrnivmodl.bat
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
export MODLUNIT=$N/lib/nrnunits.lib
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
scp hines@neuron.yale.edu:/home/htdocs/neuron/static/docs/nrnhelp.zip .
mkdir html
cd html
unzip ../nrnhelp.zip
fi
if test -d "$hparent/html" ; then
	cd $hparent
	rm -f html.zip
	zip -r html.zip html -x \*.svn\*
	rm -r -f $marshal_dir/html
	unzip -d $marshal_dir html.zip
	rm -f html.zip
fi
else #nsis requires something
mkdir $marshal_dir/html
touch $marshal_dir/html/empty
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
