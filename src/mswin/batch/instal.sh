#!/bin/sh
# install from the build directories to the mswin destination
# uses the classical positions of files

if test "$top_srcdir" = "" ; then
	echo "instal.sh should be executed at top level with make mswin"
else
	S=`cygpath -u -a $top_srcdir`
	export S
fi
if test "$marshall_dir" = "" ; then
	echo "instal.sh should be executed at top level with make mswin"
else
	mkdir $marshall_dir
	D="`cygpath -u -a $marshall_dir`"/nrn
	export D
fi

if test "$ivbindir" = "" ; then
	echo "instal.sh should be executed at top level with make mswin"
	exit 1
fi

echo "MSWIN install from $S to $D"

set -v

if true ; then # false means skip the entire marshalling of nrn

rm -r $D
mkdir $D
mkdir $D/bin
mkdir $D/lib

# copy and strip the various executables we built
cp $S/src/nrniv/mos2nrn.exe $D/bin/mos2nrn.exe
strip $D/bin/mos2nrn.exe
cp $S/src/nrniv/neuron.exe $D/bin/neuron.exe
strip $D/bin/neuron.exe
cp $S/src/mswin/nrniv.exe $D/bin/nrniv.exe
strip $D/bin/nrniv.exe
cp $S/src/mswin/hocmodule.dll $D/bin/hocmodule.dll
strip $D/bin/hocmodule.dll
cp $ivbindir/cygIVhines-3.dll $D/bin/cygIVhines-3.dll
strip $D/bin/cygIVhines-3.dll
cp $S/src/nmodl/nocmodl.exe $D/bin
strip $D/bin/nocmodl.exe
cp $S/src/modlunit/modlunit.exe $D/bin
strip $D/bin/modlunit.exe
#following could be a problem since it was not compiled -mno-cygwin
#cp $S/src/scopmath/.libs/libscopmath.a $D/lib/libscpmt.a

# copy the essential cygwin programs
for i in \
 as.exe basename.exe cat.exe cp.exe \
 cpp.exe cygpath.exe dirname.exe echo.exe find.exe gcc.exe \
 grep.exe ld.exe ls.exe make.exe mkdir.exe \
 nm.exe rm.exe mv.exe sed.exe bash.exe unzip.exe \
 rxvt.exe \
 ; do
 cp /usr/bin/$i $D/bin/$i
done
# use ash as sh to avoid missing /tmp message when cygwin not installed
cp /usr/bin/ash.exe $D/bin/sh.exe

# and there is also one in a gcc version specific place
gspec=`gcc -v 2>&1 | sed -n '/^Reading specs/{
	s;^[^/]*;;
	s;/specs;;
	p
}'`
cp $gspec/cc1.exe $D/bin

# figure out which dll's need to be distributed
(cd $D/bin
rm -f temp.tmp
for i in *.exe ; do
	cygcheck ./$i | sed 's/^ *//' >> temp.tmp
done
)
for i in ` sort $D/bin/temp.tmp | uniq | sed '
	/WINDOWS/d
	/\.exe/d
' ` ; do
	echo $i
	cp `cygpath -u $i` $D/bin
done
rm $D/bin/temp.tmp

# and there may be some others we need to do explicitly
for i in \
 libW11.dll \
 ; do
 cp /usr/bin/$i $D/bin/$i
done

mkdir $D/lib/x
cp /usr/share/terminfo/x/xterm $D/lib/x/xterm

cp $S/src/mswin/*.ico $D/bin

if 0 ; then
mkdir $D/mingw
mkdir $D/mingw/sys
cp /usr/i686-pc-mingw32/include/*.h $D/mingw
cp /usr/i686-pc-mingw32/include/sys/*.h $D/mingw/sys
( #some of the above contain merely include_next directives.
cd /usr/lib/gcc-lib/i686-pc-mingw32/3.3.1/include
cp float.h stdarg.h stddef.h varargs.h $D/mingw
)
fi

mkdir $D/gccinc
mkdir $D/gcc3inc
mkdir $D/gcclib
cp /usr/include/*.h $D/gccinc
for i in sys machine cygwin ; do
  mkdir $D/gccinc/$i
  cp /usr/include/$i/*.h $D/gccinc/$i
done
cp /usr/lib/gcc/i686-pc-cygwin/3.4.4/include/*.h $D/gcc3inc
cp /usr/lib/gcc/i686-pc-cygwin/3.4.4/libgcc.a $D/gcclib
cp /usr/lib/libcygwin.a $D/gcclib
for i in libuser32.a libkernel32.a libadvapi32.a libshell32.a ; do
  cp /usr/lib/w32api/$i $D/gcclib
done


if test -f "$S/src/nrnjava/neuron.jar" ; then
	mkdir $D/classes
	cp $S/src/nrnjava/*.jar $D/classes
fi

# copy the dos formatted files to the classical positions.
# use zip to do the translation from unix to dos format

Z=d2ufiles.zip

if true ; then
cd $S/share
rm $Z
zip -l -r $Z examples lib demo
zip -d $Z \*CVS\* \*.svn* \*Makefile\* \*.o \*.c \*.dll \*/auditscripts*
unzip -d $D -o $Z 
rm $Z
fi

if true ; then
cd $S
rm $Z
zip -l $Z src/oc/*.h src/nrnoc/*.mod src/nrnoc/*.h src/scopmath/*.h
unzip -d $D -o $Z
rm $Z
fi

if true ; then
cd $S/src/mswin
rm $Z
zip -l $Z notes.txt
#do the lib shell scripts in unix format
zip $Z bin/mknrndll lib/*.sh lib/*.mak lib/*.sed
unzip -d $D -o $Z
rm $Z
fi

if true ; then
cd /usr/lib
pz=python25.zip
pd=python2.5
if test ! -f $pz ; then
	zip -r $pz $pd
	zip -d $pz \*/\*.pyo
	zip -d $pz \*/\*.pyc
	for i in bsddb email encodings idlelib test ; do
		zip -d $pz \*/${i}*
	done
fi
unzip -d $D/lib -o $pz
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

fi # end of nrn marshalling

if true ; then # false means skip marshalling of html

hparent=$S/..
if test -d "$hparent/html" ; then
	cd $hparent
	rm html.zip
	zip -r html.zip html
	rm -r -f $marshall_dir/html
	unzip -d $marshall_dir html.zip
	rm html.zip
fi

fi # end of html marshalling

set +v
echo "Will now complete the creation of the installer by launching"
echo "    $S/src/mswin/nrnsetup.nsi ."
echo " The installer will be located in $S/src/mswin/nrnxxsetup.exe"

cd $S/src/mswin
#c:/Program\ Files/NSIS/makensisw nrnsetup.nsi
c:/Program\ Files/NSIS/makensis nrnsetup.nsi
