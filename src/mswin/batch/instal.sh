#!/bin/sh
# install from the build directories to the mswin destination
# uses the classical positions of files

if test "$top_srcdir" = "" ; then
	echo "instal.sh should be executed at top level with make mswin"
else
	S=`cygpath -u -a $top_srcdir | sed 's/\/$//'`
	export S
fi
if test "$top_builddir" = "" ; then
	echo "instal.sh should be executed at top level with make mswin"
else
	B=`cygpath -u -a $top_builddir | sed 's/\/$//'`
	export B
fi
if test "$marshal_dir" = "" ; then
	echo "instal.sh should be executed at top level with make mswin"
else
	mkdir $marshal_dir
	D="`cygpath -u -a $marshal_dir`"/nrn
	export D
fi

if test "$ivbindir" = "" ; then
	echo "instal.sh should be executed at top level with make mswin"
	exit 1
fi
echo "MSWIN install from $S and $B to $D"

set -x

if true ; then # false means skip the entire marshaling of nrn

rm -r -f $D
echo "hello"
mkdir $D
mkdir $D/bin
mkdir $D/lib
mkdir $D/tmp

# copy and strip the various executables we built
cp $B/src/nrniv/mos2nrn.exe $D/bin/mos2nrn.exe
strip $D/bin/mos2nrn.exe
cp $B/src/nrniv/neuron.exe $D/bin/neuron.exe
strip $D/bin/neuron.exe
cp $B/src/mswin/nrniv.exe $D/bin/nrniv.exe
strip $D/bin/nrniv.exe
cp $B/src/mswin/nrniv.dll $D/bin/nrniv.dll
strip $D/bin/nrniv.dll
cp $B/src/mswin/hocmodule.dll $D/bin/hocmodule.dll
strip $D/bin/hocmodule.dll
cp $ivbindir/cygIVhines-3.dll $D/bin/cygIVhines-3.dll
strip $D/bin/cygIVhines-3.dll
if test -f $B/src/nmodl/.libs/nocmodl.exe ; then
	cp $B/src/nmodl/.libs/nocmodl.exe $D/bin
	cp $B/src/modlunit/.libs/modlunit.exe $D/bin
else
	cp $B/src/nmodl/nocmodl.exe $D/bin
	cp $B/src/modlunit/modlunit.exe $D/bin
fi
strip $D/bin/nocmodl.exe
strip $D/bin/modlunit.exe

#needed by neuron.exe
if test "$host_cpu" = "x86_64" ; then
  X=/usr/x86_64-w64-mingw32/sys-root/mingw/bin
  for i in libstdc++-6.dll libgcc_s_seh-1.dll ; do
    cp $X/$i $D/bin
    strip $D/bin/$i
  done
else
  X=/usr/i686-pc-mingw32/sys-root/mingw/bin
  for i in libstdc++-6.dll libgcc_s_dw2-1.dll ; do
    cp $X/$i $D/bin
    strip $D/bin/$i
  done
fi

if test "$LTCC" = "" ; then
	LTCC=gcc
fi

# copy the essential cygwin programs
for i in \
 as.exe basename.exe cat.exe cp.exe cmp.exe which.exe \
 cpp.exe cygpath.exe diff.exe dirname.exe echo.exe find.exe $LTCC.exe \
 grep.exe ld.exe ls.exe make.exe mkdir.exe \
 nm.exe rm.exe mv.exe sed.exe bash.exe unzip.exe \
 rxvt.exe mintty.exe cygwin-console-helper.exe \
 rebase.exe sort.exe cygcheck.exe \
 ; do
 cp /usr/bin/$i $D/bin/$i
done
# use ash as sh to avoid missing /tmp message when cygwin not installed
cp /usr/bin/ash.exe $D/bin/sh.exe

# and there is also one in a gcc version specific place
cc1=`$CC -print-prog-name=cc1`
cp $cc1 $D/bin

# in case this is an mpi version distribute the appropriate administrative tools.
#nowadays (mpich-3.0.2) mpiinstalled is same as mpichbin
if grep '^mpicc=mpicc' $B/src/mswin/nrncygso.sh ; then
	mpiinstalled=$HOME/mpich2
  if test -d $mpiinstalled ; then # want mpd
	cp /bin/python2.7 $D/bin
	for i in mpdboot mpdtrace mpdexit mpdallexit mpdcleanup mpd \
	  mpiexec.mpd mpdman.py mpdlib.py \
	  mpdroot.exe; do
		sed '1s/\/usr\/bin\/env //' $mpiinstalled/bin/$i > $D/bin/$i
		chmod 755 $D/bin/$i
	done
	echo 'MPD_SECRETWORD=neuron' > $D/mpd.conf
	chmod 600 $D/mpd.conf
  fi
	# gforker
	cp $mpiinstalled/bin/mpiexec.exe $D/bin
	cp $mpiinstalled/lib/libmpich.dll $D/bin
	# and make the basic tests available
	for i in test0.hoc test0.py ; do
		cp $S/src/parallel/$i $D
		unix2dos $D/$i
	done
fi

if true ; then  #moved to here to allow figuring out what dlls are needed
cd /usr/lib
pz=python27.zip
pd=python2.7
if test ! -f $pz ; then
	zip -r $pz $pd
	zip -d $pz \*/\*.pyo
	zip -d $pz \*/\*.pyc
	for i in bsddb email encodings idlelib test ; do
		zip -d $pz \*/${i}/\*
	done
	for i in __init__ aliases ascii ; do
		zip $pz $pd/encodings/$i.py
	done
fi
unzip -d $D/lib -o $pz
#to avoid site problem
mkdir -p $D/usr/include/python2.7
cp /usr/include/python2.7/pyconfig.h $D/usr/include/python2.7
# crazy but need it here as well and do not really know why.
mkdir -p $D/include/python2.7
cp /usr/include/python2.7/pyconfig.h $D/include/python2.7
#if continue to have problems with this then perhaps explicitly set Py_NoSiteFlag
fi

# figure out which dll's need to be distributed
(cd $D/bin
rm -f temp.tmp
for i in *.exe ; do
	cygcheck ./$i | sed 's/^ *//' >> temp.tmp
done
)
# do not forget the ones used by the python dlls
# only the python dlls in $D/lib/python2.7
if true ; then
# too many, putting duplicates in bin, setup is 13.69MB, only cygcrypto below
for i in `find $D/lib/python2.7 -name \*.dll` ; do
	cygcheck $i | sed 's/^ *//' >> $D/bin/temp.tmp
done
fi

sort $D/bin/temp.tmp | uniq | grep 'cygwin[0-9]*\\bin\\' | sed 's,\\,/,g' > $D/temp2.tmp
for i in `cat $D/temp2.tmp` ; do
	echo $i
	cp `cygpath -u $i` $D/bin
done
rm $D/bin/temp.tmp
rm $D/bin/temp2.tmp
chmod 755 $D/bin/*
chmod 755 `find $D/lib -name \*.dll`

# and there may be some others we need to do explicitly
for i in \
 libW11.dll \
 ; do
 cp /usr/bin/$i $D/bin/$i
done

#mkdir $D/lib/x
#cp /usr/share/terminfo/x/xterm $D/lib/x/xterm
#as of cygwin 7 it is here
mkdir $D/usr
mkdir $D/usr/share
mkdir $D/usr/share/terminfo
mkdir $D/usr/share/terminfo/78
cp /usr/share/terminfo/78/xterm $D/usr/share/terminfo/78/xterm

cp $S/src/mswin/*.ico $D/bin
cp /usr/lib/default-manifest.o $D/bin

if false ; then
mkdir $D/mingw
mkdir $D/mingw/sys
cp /usr/i686-pc-mingw32/include/*.h $D/mingw
cp /usr/i686-pc-mingw32/include/sys/*.h $D/mingw/sys
( #some of the above contain merely include_next directives.
cd /usr/lib/gcc-lib/i686-pc-mingw32/3.3.1/include
cp float.h stdarg.h stddef.h varargs.h $D/mingw
)
fi

gclib=`$CC -print-libgcc-file-name`
gclib=`echo $gclib|sed 's,.*/lib/\(.*\)/[^/].*,\1,'`
echo $gclib
(cd /usr/lib ; zip -r $D/lib/temp.tmp $gclib -x \*ada\* \*c++\* \*fortran\* \*libobj\* \*libgcj\* \*install-tools\* \*libff\* \*libgomp\* \*libgij\* \*finclude\*)
(cd $D/lib ; unzip temp.tmp ; rm temp.tmp ; cd $gclib ; rm -f cc1.exe *obj* *plus* e* j* f* gnat* *ssp* )

mkdir $D/gccinc
cp /usr/include/*.h $D/gccinc
for i in sys machine cygwin bits ; do
  mkdir $D/gccinc/$i
  cp /usr/include/$i/*.h $D/gccinc/$i
done

mkdir $D/gcclib
cp /usr/lib/libcygwin.a $D/gcclib
cp /usr/lib/libpthread.a $D/gcclib
for i in libuser32.a libkernel32.a libadvapi32.a libshell32.a ; do
  cp /usr/lib/w32api/$i $D/gcclib
done

if false ; then
mkdir $D/gcc3inc
cp /usr/lib/gcc/i686-pc-cygwin/3.4.4/include/*.h $D/gcc3inc
cp /usr/lib/gcc/i686-pc-cygwin/3.4.4/libgcc.a $D/gcclib
fi

if test -f "$S/src/nrnjava/neuron.jar" ; then
	mkdir $D/classes
	cp $S/src/nrnjava/*.jar $D/classes
fi

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
rm -f $Z
zip -l $Z notes.txt
#do the lib shell scripts in unix format
zip $Z bin/mknrndll bin/neurondemo bin/nrngui lib/*.sh lib/*.bsh lib/minttyrc lib/*.sed
#do the specified unix bin shell scripts in unix format
cd $S
zip $Z bin/mkthreadsafe bin/nrnpyenv.sh bin/set_nrnpyenv.sh bin/sortspike
unzip -d $D -o $Z
rm $Z
cd $B/src/mswin
rm -f $Z
#do the lib shell scripts in unix format
zip $Z lib/*.mak
unzip -d $D -o $Z
rm $Z
fi

if true ; then
cp $D/bin/hocmodule.dll $D/lib/python/neuron/hoc.dll
cp $D/bin/mknrndll $D/bin/nrnivmodl
fi

if true ; then
cd $S/src/mswin
mv $D/lib/nrn.defaults $D/lib/nrn.def
fi

cd $D/demo/release
export N=$D
export PATH="$D/bin:$PATH"
sh $D/lib/mknrndl2.sh
rm *.o

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

cd $B/src/mswin
#c:/Program\ Files/NSIS/makensisw nrnsetup.nsi
if test "$host_cpu" = "x86_64" ; then
  c:/Program\ Files\ \(x86\)/NSIS/makensis nrnsetup.nsi
else
  c:/Program\ Files/NSIS/makensis nrnsetup.nsi
fi
