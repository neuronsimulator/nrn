#!/usr/bin/env bash

#Work around for the problem that
#I cannot get the linker to use /usr/X11/lib instead of /opt/X11/lib
#The issue is with Mavericks (10.7) that has an apple version of X11 already
#installed in /usr/X11 instead of xquartz which installs in /opt/X11 and has
#a symbolic link to /usr/X11

#Change all links of /opt/X11/lib/*.dyld to /usr/X11/lib/*.dyld

prefix=$1
cpu=$2

function change() {
  a="`otool -L $1 |sed -n 's,.*/opt/X11/lib/\([^ ]*\) .*,\1,p'`"
  if test "$a" != "" ; then
    for name in $a ; do
      echo change $name in $i
      install_name_tool -change /opt/X11/lib/$name /usr/X11/lib/$name $i
    done
  fi
}

cd $prefix
echo $prefix
echo $cpu

for i in `find . -name \*.so` ; do
  change $i
done
for i in `find . -name \*.dylib` ; do
  change $i
done

if test -d $cpu/bin ; then
  cd $cpu/bin
else
  cd bin
fi
for i in * ; do
  change $i
done

cd $prefix
if test -d ../iv ; then
  cd ../iv/$cpu
  cd bin
  for i in * ; do
    change $i
  done
  cd ../lib
  for i in * ; do
    change $i
  done
fi
