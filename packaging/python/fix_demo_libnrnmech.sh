#!/usr/bin/env bash
set -ex

# On Mac, update neurondemo libnrnmech to point to relative libnrniv.
# neurondemo dlopens ..../release/x86_64/.libs/libnrnmech.so
# Remove the libnrnmech.so if it is a symbolic link
#  and copy ..../release/x86_64/libnrnmech.dylib to ...../lib/libnrnmech.so
# Rewrite @rpath/libnrniv.dylib to @loader_path/...../lib/libnrniv.dylib

instdir=$1
REL_RPATH=$2
libnrnmech_dir="$instdir/share/nrn/demo/release"
if [ -d "$libnrnmech_dir/arm64" ]; then
  libnrnmech_dir="$libnrnmech_dir/arm64"
else
  libnrnmech_dir="$libnrnmech_dir/x86_64"
fi

if test "`uname -s`" = "Darwin" ; then
  if test -h "$libnrnmech_dir/.libs/libnrnmech.so" ; then
    rm -f $libnrnmech_dir/.libs/libnrnmech.so
    mv $libnrnmech_dir/libnrnmech.dylib $libnrnmech_dir/.libs/libnrnmech.so
  fi
  install_name_tool -change '@rpath/libnrniv.dylib' \
    '@loader_path/../../../../../../lib/libnrniv.dylib' \
    $libnrnmech_dir/.libs/libnrnmech.so
fi


