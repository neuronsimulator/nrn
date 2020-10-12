#!/bin/sh
set -ex

# On mac delocate-wheel rewrites the location for libnrniv to a copy of
# that in neuron/.dylibs/libnrniv.dylib. This causes a segfault.
# Here we rewrite again to find neuron/.data/lib/libnrniv.dylib
# Although neurondemo only dlopens ..../release/x86_64/.libs/libnrnmech.so
# we also rewrite the copy in ..../release/x86_64/libnrnmech.dylib

if test "`uname -s`" != "Darwin" ; then
  echo "ERROR: demo_update_libnrnmech.sh only for Darwin"
  exit 1
fi

WHEELHOUSE=$1
NRNWHEEL=$2

cd $WHEELHOUSE

if test ! -f $NRNWHEEL ; then
  echo "ERROR: NRNWHEEL $NRNWHEEL not in WHEELHOUSE $WHEELHOUSE"
  exit 1
fi

rm -r -f neuron
unzip $NRNWHEEL neuron/.data/share/nrn/demo/release/x86_64/.libs/libnrnmech.so
install_name_tool -change '@loader_path/../../../../../../../.dylibs/libnrniv.dylib' '@loader_path/../../../../../../lib/libnrniv.dylib' neuron/.data/share/nrn/demo/release/x86_64/.libs/libnrnmech.so
zip $NRNWHEEL neuron/.data/share/nrn/demo/release/x86_64/.libs/libnrnmech.so
unzip $NRNWHEEL neuron/.data/share/nrn/demo/release/x86_64/libnrnmech.dylib
install_name_tool -change '@loader_path/../../../../../../.dylibs/libnrniv.dylib' '@loader_path/../../../../../lib/libnrniv.dylib' neuron/.data/share/nrn/demo/release/x86_64/libnrnmech.dylib
zip $NRNWHEEL neuron/.data/share/nrn/demo/release/x86_64/libnrnmech.dylib
rm -r -f neuron
