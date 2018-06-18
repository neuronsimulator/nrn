#!/bin/sh
set -ex

# if arg then assume it is destination, eg, c:/marshalnrn64/nrn
if test $# = 1 ; then
  N=$1/mingw
else
  N='c:/nrn/mingw'
  rm -r -f $N
fi

#basic environment

mkdir -p $N/usr/bin
mkdir -p $N/etc
mkdir -p $N/tmp
mkdir -p $N/usr/share/terminfo/63
cp /usr/share/terminfo/63/* $N/usr/share/terminfo/63
cp $HOME/.inputrc $N/etc/inputrc

#cp /msys2.ico $N
#cp /msys2.ini $N
#cp /msys2_shell.cmd $N

binprog="basename bash cat cp dirname echo find grep ls make mintty
  mkdir mv rebase rm sed sh sort unzip which"
for i in $binprog ; do
  echo $i
  cp /usr/bin/$i.exe $N/usr/bin/$i.exe
done

(
  rm -f temp.tmp
  for i in $N/usr/bin/*.exe ; do
    cygcheck $i | sed 's/^ *//' >> temp.tmp
  done
  sort temp.tmp | uniq | grep 'msys64' | sed 's,\\,/,g' > temp2.tmp
  for i in `cat temp2.tmp` ; do
    echo $i
    cp $i $N/usr/bin
  done
  rm temp.tmp
  rm temp2.tmp
)

# minimal gcc build system for mknrndll
# this portion of the file started life as
# $ (cd c:/nrn/mingw ; ls -R mingw64) > nrngcc64.sh

M=$N/mingw64
X=x86_64-w64-mingw32
gccver=`gcc --version | sed -n '1s/.* //p'`

# all the folders involved that have files
mkdir -p $M/bin
mkdir -p $M/lib/gcc/$X/$gccver
mkdir -p $M/$X/lib
mkdir -p $M/$X/include/sdks
mkdir -p $M/$X/include/sec_api/sys
mkdir -p $M/$X/include/sys

copy() {
  for i in $2 ; do
    cp /$1/$i $N/$1/$i
  done
}

copy mingw64/bin '
as.exe
ld.exe
libgmp-10.dll
x86_64-w64-mingw32-gcc.exe
zlib1.dll
'

copy mingw64/lib/gcc/x86_64-w64-mingw32/$gccver '
cc1.exe
libgcc.a
libgcc_s.a
liblto_plugin-0.dll
'

copy mingw64/x86_64-w64-mingw32/include '
_mingw.h
_mingw_mac.h
_mingw_off_t.h
_mingw_print_pop.h
_mingw_print_push.h
_mingw_secapi.h
assert.h
corecrt_startup.h
crtdefs.h
errno.h
limits.h
malloc.h
math.h
process.h
pthread.h
pthread_compat.h
pthread_signal.h
pthread_unistd.h
signal.h
stdio.h
stddef.h
stdlib.h
string.h
swprintf.inl
vadefs.h
'

copy mingw64/x86_64-w64-mingw32/include/sdks '
_mingw_ddk.h
_mingw_directx.h
'

copy mingw64/x86_64-w64-mingw32/include/sec_api '
stdio_s.h
stdlib_s.h
string_s.h
sys/timeb_s.h
'

copy mingw64/x86_64-w64-mingw32/include/sys '
timeb.h
types.h
'

copy mingw64/x86_64-w64-mingw32/lib '
crtbegin.o
crtend.o
dllcrt2.o
libadvapi32.a
libkernel32.a
libmingw32.a
libmingwex.a
libmoldname.a
libmsvcrt.a
libpthread.a
libpthread.dll.a
libshell32.a
libuser32.a
'
