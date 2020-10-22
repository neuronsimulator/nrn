#!/bin/sh
set -ex

# On Mac, update neurondemo libnrnmech to point to relative libnrniv.
# neurondemo dlopens ..../release/x86_64/.libs/libnrnmech.so
# Remove the libnrnmech.so symbolic link.
# Copy ..../release/x86_64/libnrnmech.dylib to ...../lib/libnrnmech.so
# Rewrite @rpath/libnrniv.dylib to @loader_path/...../lib/libnrniv.dylib

instdir=$1
REL_RPATH=$2
libnrnmech_dir=$instdir/share/nrn/demo/release/x86_64

if test "`uname -s`" = "Darwin" ; then
  if test ! -h "$libnrnmech_dir/.libs/libnrnmech.so" ; then
    echo "$libnrnmech_dir/.libs/libnrnmech.so is not a symbolic link"
    exit 1
  fi
  rm -f $libnrnmech_dir/.libs/libnrnmech.so
  mv $libnrnmech_dir/libnrnmech.dylib $libnrnmech_dir/.libs/libnrnmech.so
  install_name_tool -change '@rpath/libnrniv.dylib' \
    '@loader_path/../../../../../../lib/libnrniv.dylib' \
    $libnrnmech_dir/.libs/libnrnmech.so
fi


