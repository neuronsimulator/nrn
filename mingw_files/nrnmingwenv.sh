#!/bin/bash

set -ex

# Copies minimal compiler toolchain
# to allow nrnivmodl (mknrndll) to build nrnmech.dll .

# if arg then assume it is destination, eg, c:/marshalnrn64/nrn
if test $# -ge 1 ; then
  N="$1"
else
  N='c:/nrn'
  rm -r -f "$N/mingw"
fi

NM=$N/mingw

#basic environment

mkdir -p "$NM/usr/bin"
mkdir -p "$NM/etc"
mkdir -p "$NM/tmp"
mkdir -p "$NM/usr/share/terminfo/78"
cp /usr/share/terminfo/78/* $NM/usr/share/terminfo/78
cp $HOME/.inputrc $NM/etc/inputrc

#cp /msys2.ico $NM
#cp /msys2.ini $NM
#cp /msys2_shell.cmd $NM

binprog="basename bash cat cp dirname echo find grep ls make mintty
  mkdir mv rebase rm sed sh sort unzip which cygpath cygcheck uname"
for i in $binprog ; do
  echo $i
  cp /usr/bin/$i.exe $NM/usr/bin/$i.exe
done

# first arg is the folder containing the *.exe .
# Second arg is the folder into which to copy the dlls.
cp_dlls() {
(
  upath=`cygpath "$1"`
  export PATH="$upath":$PATH
  rm -f temp.tmp
  for i in $1/*.exe ; do
    cygcheck $i | sed 's/^ *//' >> temp.tmp
  done
  sort temp.tmp | uniq | grep 'msys64' | sed 's,\\,/,g' > temp2.tmp
  for i in $(cat temp2.tmp) ; do
    echo $i
    cp "$i" "$2"
  done
  rm temp.tmp
  rm temp2.tmp
)
}

cp_dlls $NM/usr/bin $NM/usr/bin

# minimal gcc build system for mknrndll
# this portion of the file started life as
# $ (cd c:/nrn/mingw ; ls -R mingw64) > nrngcc64.sh

M=$NM/mingw64
X=x86_64-w64-mingw32
gccver=`gcc --version | sed -n '1s/.* //p'`
if test ! -d "$gccver" ; then
  gccver=`gcc --version | sed -n '1s/.*)  *\([^ ]*\).*/\1/p'` 
fi

# all the folders involved that have files
mkdir -p "$M/bin"
mkdir -p "$M/lib/gcc/$X/$gccver/include"
mkdir -p "$M/$X/lib"
mkdir -p "$M/$X/include/sdks"
mkdir -p "$M/$X/include/sec_api/sys"
mkdir -p "$M/$X/include/sys"

copy() {
  for i in $2 ; do
    cp "/$1/$i" "$NM/$1/$i"
  done
}

copy mingw64/bin '
as.exe
ld.exe
x86_64-w64-mingw32-gcc.exe
'
cp_dlls $NM/mingw64/bin $NM/mingw64/bin

copy mingw64/lib/gcc/x86_64-w64-mingw32/$gccver '
cc1.exe
libgcc.a
libgcc_s.a
liblto_plugin-0.dll
'
cp_dlls $NM/mingw64/lib/gcc/x86_64-w64-mingw32/$gccver $NM/mingw64/bin
rm -f $NM/mingw64/bin/libwinpthread-1.dll # already in $N/bin

# copy all needed include files by processing output of gcc -E
copyinc() {
  echo "" > temp.c
  for i in $* ; do
    echo "#include <$i>"
  done >> temp.c
  echo "int main(int argc, char** argv){return 0;}" >> temp.c
  gcc -E temp.c  | grep '^#.*include' > temp1
  sed -n 's,^.*msys64/,,p' temp1 | sed -n 's,".*,,p' > temp2
  sort temp2 | uniq > temp3
  sed -n 's,/[^/]*$,,p' temp3 | sort  | uniq > temp4
  for i in $(cat temp4) ; do
    mkdir -p "$NM/$i"
  done
  for i in $(cat temp3) ; do
    cp /$i "$NM/$i"
  done
}

copyinc '
_mingw.h
_mingw_mac.h
_mingw_off_t.h
_mingw_secapi.h
assert.h
corecrt.h
corecrt_startup.h
corecrt_wstdlib.h
crtdefs.h
errno.h
inttypes.h
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
stdint.h
stdlib.h
string.h
swprintf.inl
time.h
unistd.h
vadefs.h
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
